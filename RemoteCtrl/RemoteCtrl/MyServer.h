#pragma once
#include "MyThread.h"
#include "pch.h"
#include <map>

class MyClient {

};

enum ServerOperator{
    MNone,
    MAccept,
    MRecv,
    MSend,
    MError
};

class CBaseOverlapped
{
public:
    DWORD m_operator;       // enum ServerOperator
    OVERLAPPED m_overlapped;
    std::vector<char> m_buffer;
    ThreadWorker m_worker;    // 处理函数
};

template<ServerOperator>
class AcceptOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped() :m_operator(MAccept), m_worker(this, &AcceptOverlapped::AcceptWorker) {
        memset(m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }

    int AcceptWorker() {
        // TODO:
    }
};
typedef AcceptOverlapped<MAccept> ACCEPTOVERLAPPED;


template<ServerOperator>
class RecvOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    RecvtOverlapped() :m_operator(MRecv), m_worker(this, &RecvOverlapped::RecvWorker) {
        memset(m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
    }

    int RecvWorker() {
        // TODO:
    }
};
typedef RecvOverlapped<MRecv> RECVOVERLAPPED;


template<ServerOperator>
class SendOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    SendOverlapped() :m_operator(MSend), m_worker(this, &SendOverlapped::SendWorker) {
        memset(m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
    }

    int SendWorker() {
        // TODO:
    }
};
typedef SendOverlapped<MSend> SENDOVERLAPPED;


template<ServerOperator>
class ErrorOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() :m_operator(MError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
    }

    int ErrorWorker() {
        // TODO:
    }
};
typedef ErrorOverlapped<MError> ERROROVERLAPPED;


class CMyServer :
    public ThreadFuncBase
{
public:
    CMyServer(const char* IP = "0.0.0.0" , short Port = 9527): threadPool(10){
        m_hIocp = INVALID_HANDLE_VALUE;
        servSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        int opt = -1;
        setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in servAddr;
        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(Port);
        servAddr.sin_addr.s_addr = inet_addr(IP);

        if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
            closesocket(servSock);
            servSock = INVALID_SOCKET;
            return;
        }

        if (listen(servSock, 5) == SOCKET_ERROR) {
            closesocket(servSock);
            servSock = INVALID_SOCKET;
            return;
        }

        m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        CreateIoCompletionPort((HANDLE)servSock, m_hIocp, (ULONG_PTR)this, 0);
        if (m_hIocp == NULL) {
            m_hIocp = INVALID_HANDLE_VALUE;
            closesocket(servSock);
            servSock = INVALID_SOCKET;
            return;
        }

        // 每次创建一个servSock，就会调用threadIocp，接收端口完成。
        threadPool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CMyServer::threadIocp));
    }

    ~CMyServer(){}
private:
    int threadIocp() {
        LPOVERLAPPED lpOverlapped = NULL;
        DWORD transferred = 0;
        ULONG_PTR key = 0;

        if(GetQueuedCompletionStatus(m_hIocp, &transferred, &key, &lpOverlapped, INFINITE))
        {
            if (transferred != 0 && (key != 0))
            {
                CBaseOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, CBaseOverlapped, m_overlapped);
                switch (pOverlapped->m_operator) {
                case MAccept:
                {
                    ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                    threadPool.DispatchWorker(pOver->m_worker);
                }
                break;

                case MRecv:
                {
                    RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                    threadPool.DispatchWorker(pOver->m_worker);
                }
                break;

                case MSend:
                {
                    SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                    threadPool.DispatchWorker(pOver->m_worker);
                }
                break;

                case MError:
                {
                    ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                    threadPool.DispatchWorker(pOver->m_worker);
                }
                break;

                }
            }
            else {
                return -1;
            }
            
        }
    }

private:
    CMyThreadPool threadPool;
    SOCKET servSock;
    HANDLE m_hIocp;

    std::map<SOCKET, std::shared_ptr<MyClient>> m_clients;
};

