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


bool CClientSocket::SendPacket(CPacket pack, std::list<CPacket>& recvPackets)
{
	if (cli_sock == INVALID_SOCKET) {
		// 正式发送包的时候，IP和Port会固定下来了。
		if (InitSocket() == false) return false;
		//启动线程
		_beginthread(&CClientSocket::threadEntry, 0, this);
	}

	m_listSendPacket.push_back(pack);
	WaitForSingleObject(pack.hEvent, INFINITE);

	auto it = m_mapRecvPacket.find(pack.hEvent);
	if (it != m_mapRecvPacket.end()) {
		for (auto i = it->second.begin(); i != it->second.end(); i++) {
			recvPackets.push_back(*i);
		}
		m_mapRecvPacket.erase(it);
		return true;
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
			auto it = m_mapRecvPacket.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));

			int length = recv(cli_sock, pBuffer + index, BUFFER_SIZE - index, 0);

			if (length > 0 || index > 0) {
				index += length;
				size_t size = (size_t)index;
				CPacket pack((BYTE*)pBuffer, size);
				if (size > 0) {
					index -= size;

					pack.hEvent = head.hEvent;
					it.first->second.push_back(pack);
					SetEvent(head.hEvent);
				}
			}
			else if (length <= 0 && index <= 0) 
			{
				CloseSocket();
			}
			//TODO	待解决的问题 对于获取文件夹消息的命令，只会在第一个包存在hEvent，删除后，之后包就无法通知了。
			m_listSendPacket.pop_front();
		}
	}
	CloseSocket();
}
