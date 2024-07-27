#pragma once

#include <string>
#include <vector>
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)

void Dump(BYTE* pData, size_t nSize);


class CPacket
{
public: 
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (int j = 0; j < nSize; j++) {
			sSum += BYTE(pData[j]) & 0xFF;
		}
	}

	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}

	//nSize传入是数据大小，传出是获取的数据量
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		//遍历pData，找到以0xFEFF开头的包头。
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		//可以等于nSize，只有控制命令，无包数据。
		if (i + 4 + 2 + 2 > nSize) {	//包数据可能不全，或者包头未能全部接收到。
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (i + nLength > nSize) {		//可能没有对大数据包进行 发送端 分包处理。nLength记录着整个包的长度。
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			//TRACE("[客户端]%s\r\n", strData.c_str() + 12);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;

		WORD temp = 0;
		for (int j = 0; j < strData.size(); j++) {
			temp += BYTE(strData[j]) & 0xFF;
		}

		if (temp == sSum) {
			//nSize = nLength + 2 + 4;
			nSize = i;						//包头前面可能有无效数据，得记录全部，并传出。
			return;
		}
		nSize = 0;
	}

	~CPacket() {}

	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() {	//数据的大小
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(Size());
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;	pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd;	pData += 2;
		memcpy(pData, strData.c_str(), strData.size());	pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	} 

public:
	WORD sHead;
	DWORD nLength;
	WORD sCmd;
	std::string strData;
	WORD sSum;
	std::string strOut;
};
#pragma pack(pop)

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;         //是否有效
	BOOL IsDirectory;       //是否为目录 0否 1是 -1无效（默认）
	BOOL HasNext;           //是否还有后续 0没有 1有（默认）
	char szFileName[256];   //文件名 
}FILEINFO, * PFILEINFO;


typedef struct MouseEvent {
	MouseEvent() {
		nAction = -1;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//点击0，双击1，按下2，放开3，无操作4，滚轮上滑5，滚轮下滑6
	WORD nButton;	//左键0，右键1，中键2
	POINT ptXY;		//坐标
}MOUSEEV, * PMOUSEEV;

//防止多次引用。
std::string GetErrInfo(int wsaErrCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) {
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	//无限次调用
	//bool InitSokcet(const std::string& serverIPAddrness) {
	bool InitSokcet(int nIP, int nPort) {
		if (cli_sock != INVALID_SOCKET) {
			CloseSocket();
		}
		cli_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (cli_sock == -1) return false;
		sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;
		//server_addr.sin_addr.s_addr = inet_addr(serverIPAddrness.c_str());
		//server_addr.sin_port = htons(9527);
		
		//server_addr.sin_addr.s_addr = nIP;			//字节序问题！
		server_addr.sin_addr.s_addr = htonl(nIP);	
		server_addr.sin_port = htons(nPort);
		
		if (server_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox(_T("指定的Ip地址不存在！"));
			return false;
		}

		int ret = connect(cli_sock, (sockaddr*)&server_addr, sizeof(sockaddr));
		
		if (ret == -1) {
			AfxMessageBox(_T("conncet连接失败！"));
			TRACE("[客户端]连接失败：%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}

#define BUFFER_SIZE 40960000

	int DealCommand() {
		if (cli_sock == -1) return -1;
		//客户端会收到服务端多个数据包
		//char* buffer = new char[BUFFER_SIZE];
		char* buffer = m_buffer.data();

		//处理完一个文件命令之后，index一定会回到0。如果后续再次点击文件发生错误，那么就是上一次的命令出现了bug
		//某几个参数写错了的话，bug得找半天。
		static size_t index = 0;		
		while (true) {
			size_t len = recv(cli_sock, buffer + index, BUFFER_SIZE - index, 0);
			TRACE("[客户端]index = %d len = %d buffer_size = %d\n", index, len, index + len);

			// 1. 之前buffer，index局部置零，会间接性的丢数据。
			// 2. 特别是，index不为0的情况下，（可能最后几个包一起接收到了，再后几次的recv会为0，index还有数据）。
			if (len <= 0 && index == 0) {
				return -1;
			}
			index += len;
			len = index;
			
			//Dump((BYTE*)buffer, len);
			m_packet = CPacket((BYTE*)buffer, len);
			
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, int nSize) {
		if (cli_sock == -1) return false;
		return send(cli_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (cli_sock == -1) return false;
		return send(cli_sock, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(cli_sock);
		cli_sock = INVALID_SOCKET;
	}

private:
	std::vector<char> m_buffer;

	//类内声明
	static CClientSocket* m_instance;
	SOCKET cli_sock;
	CPacket m_packet;
	

	CClientSocket& operator=(const CClientSocket& ss) {}

	//拷贝构造
	CClientSocket(const CClientSocket& ss) {
		cli_sock = ss.cli_sock;
	}

	CClientSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误！"), MB_OK | MB_ICONERROR);	// MB_ICONERROR消息框中会出现一个停止标志图标。
			exit(0);
		}
		//客户端需要在InitSock里面初始化cli_sock
		//cli_sock = socket(PF_INET, SOCK_STREAM, 0);
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}

	~CClientSocket() {
		//closesocket(cli_sock);
		WSACleanup();
	}

	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance() {
		if (m_instance != NULL) {
			CClientSocket* temp = m_instance;
			m_instance = nullptr;
			delete temp;
		}
	}

	class CHelper 
	{
	public:
		CHelper() {
			CClientSocket::getInstance();
		}

		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

