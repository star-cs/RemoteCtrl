#pragma once

#include "pch.h"
#include "framework.h"

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

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(9527);
		serv_addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(serv_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
			return false;
		}

		if (listen(serv_sock, 1) == -1) {
			return false;
		}

		return true;
	}

	bool AcceptClient() {
		cli_sock = accept(serv_sock, (sockaddr*)&cli_addr, &cli_sz);
		if (cli_sock == -1)	return false;
		return true;
		//recv(cli_sock, buffer, sizeof(buffer), 0);
		//send(cli_sock, buffer, sizeof(buffer), 0);
	}

	int DealCommand() {
		if (cli_sock == -1) return -1;
		while (true) {
			int len = recv(cli_sock, buffer, sizeof(buffer), 0);
			if (len <= 0) {
				return -1;
			}
			// TODO:处理命令
		}
	}

	bool Send(const char* pData, int nSize) {
		if (cli_sock == -1) return false;
		return send(cli_sock, buffer, sizeof(buffer), 0) > 0;
	}

private:
	SOCKET serv_sock;
	SOCKET cli_sock;
	sockaddr_in serv_addr;
	sockaddr_in cli_addr;
	int cli_sz = sizeof(cli_addr);
	char buffer[1024] = "";


	CServerSocket& operator=(const CServerSocket& ss) {}

	//拷贝构造
	CServerSocket(const CServerSocket& ss) {
		serv_sock = ss.serv_sock;
		cli_sock = ss.cli_sock;
	}

	//单例
	CServerSocket() {
		if (InitSockEnv() == FALSE)
		{
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