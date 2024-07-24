#pragma once

#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)	//按照一字节对齐，解决CC的问题

void Dump(BYTE* pData, size_t nSize);

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}

	//nSize 数据大小
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

	~CPacket(){}

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
	//unsigned short	WORD
	//unsigned long		DWORD

	WORD sHead;		//固定为，0xFEFF							2
	DWORD nLength;	//包长度 从控制命令开始，到和检验结束		4
	WORD sCmd;		//控制命令								2
	std::string strData;	//包数据							
	WORD sSum;		//和校验	 校验包数据						2
	std::string strOut;
};
#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = -1;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//点击0，双击1，按下2，放开3
	WORD nButton;	//左键0，右键1，中键2
	POINT ptXY;		//坐标
}MOUSEEV, *PMOUSEEV;

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

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket() {
		if (serv_sock == -1)	return false;
		sockaddr_in serv_addr;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(9527);
		serv_addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(serv_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
			printf("%d\r\n", GetLastError());
			return false;
		}

		if (listen(serv_sock, 1) == -1) {
			return false;
		}

		return true;
	}

	bool AcceptClient() {
		sockaddr_in cli_addr;
		int cli_sz = sizeof(cli_addr);
		cli_sock = accept(serv_sock, (sockaddr*)&cli_addr, &cli_sz);
		if (cli_sock == -1)	return false;
		return true;
	}

#define BUFFER_SIZE 4096

	//获取命令
	int DealCommand() {
		if (cli_sock == -1) return -1;
		//char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];	//规定好，服务端接收客户端的数据，短连接。每次只会有一条指令
		memset(buffer, 0, sizeof(buffer));
		size_t index = 0;		
		while (true) {
			size_t len = recv(cli_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[]buffer;
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);

			// TODO 
			if (len > 0) {
				//void *memmove(void *dest, const void *src, size_t n);
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;				//可以理解为，前一次解包，未处理剩余字节的个位。
				delete[]buffer;
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
		//Dump((BYTE*)pack.Data(), pack.Size());
		//(const char*)&pack 这种转换的目的是为了能够以字节流的形式访问 pack 实例中的数据。
		//return send(cli_sock, (const char*)&pack, pack.nLength + 2 + 4, 0) > 0;
		return send(cli_sock, pack.Data(), pack.Size(), 0) > 0;
	}

	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4) || (m_packet.sCmd == 9))
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

	void CloseClient() {
		closesocket(cli_sock);
		cli_sock = INVALID_SOCKET;
	}

private:
	SOCKET serv_sock;
	SOCKET cli_sock;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket& ss) {}

	//拷贝构造
	CServerSocket(const CServerSocket& ss) {
		serv_sock = ss.serv_sock;
		cli_sock = ss.cli_sock;
	}

	//单例
	CServerSocket() {
		if (InitSockEnv() == FALSE){
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误！"), MB_OK | MB_ICONERROR);	// MB_ICONERROR消息框中会出现一个停止标志图标。
			exit(0);
		}
		serv_sock = socket(PF_INET, SOCK_STREAM, 0);
		cli_sock = INVALID_SOCKET;
	};

	~CServerSocket() {
		closesocket(serv_sock);
		WSACleanup();
	};

	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	
	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* temp = m_instance;
			m_instance = nullptr;
			delete temp;
		}
	}

	//类内声明
	static CServerSocket* m_instance;

	class CHelper
	{
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;		// 静态成员变量:
};