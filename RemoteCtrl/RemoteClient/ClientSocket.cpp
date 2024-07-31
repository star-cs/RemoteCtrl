#include "pch.h"
#include "ClientSocket.h"


CClientSocket* CClientSocket::m_instance = NULL;

CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* cclient = CClientSocket::getInstance();


std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}


bool CClientSocket::SendPacket(CPacket pack, std::list<CPacket>& recvPackets, bool isAutoClosed)
{
	if (cli_sock == INVALID_SOCKET) {
		// 正式发送包的时候，IP和Port会固定下来了。
		//if (InitSocket() == false) return false;
		//启动线程
		_beginthread(&CClientSocket::threadEntry, 0, this);
	}
	
	m_mapRecvPacket.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, recvPackets));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	TRACE("cmd:%d event:%08X threadId:%ld \r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	m_listSendPacket.push_back(pack); 
	WaitForSingleObject(pack.hEvent, INFINITE);

	auto it = m_mapRecvPacket.find(pack.hEvent);
	if (it != m_mapRecvPacket.end()) {
		m_mapRecvPacket.erase(it);
	}

	auto it0 = m_mapAutoClosed.find(pack.hEvent);
	if (it0 != m_mapAutoClosed.end()) {
		m_mapAutoClosed.erase(it0);
	}

	return false;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
	ExitThread(0);
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();

	int index = 0;
	
	InitSocket();

	while (cli_sock != INVALID_SOCKET)
	{
		if (m_listSendPacket.size() > 0) 
		{
			TRACE("[客户端] m_listSendPacket.size() = %d \r\n", m_listSendPacket.size());
			CPacket& head = m_listSendPacket.front();
			if (Send(head) == false)
			{
				TRACE("发送失败\r\n");
				continue;
			}

			auto it = m_mapRecvPacket.find(head.hEvent);
			if (it != m_mapRecvPacket.end())
			{
				auto it0 = m_mapAutoClosed.find(head.hEvent);
				do {
					int length = recv(cli_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("[客户端] index = %d , length = %d, AutoClose = %d \r\n", index, length, (int)it0->second);
					
					if (length > 0 || index > 0) {
						index += length;
						size_t size = (size_t)index;
						CPacket pack((const BYTE*)pBuffer, size);
						if (size > 0) {
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;

							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							if (it0->second)
							{
								SetEvent(head.hEvent);
							}
						}
					}
					else if (length <= 0 && index <= 0)
					{
						CloseSocket();
						SetEvent(head.hEvent);
						m_mapAutoClosed.erase(it0);
						break;
					}
				} while (it0->second == false);

				m_listSendPacket.pop_front();
				
				if (InitSocket() == false) InitSocket();
			}
		}
	}
	CloseSocket();
}
