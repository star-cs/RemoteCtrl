#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;

//类外定义

CServerSocket* CServerSocket::m_instance = NULL;

CServerSocket::CHelper CServerSocket::m_helper;			//利用析构函数。解决 m_instance 不会析构的问题。

CServerSocket* cserver = CServerSocket::getInstance();