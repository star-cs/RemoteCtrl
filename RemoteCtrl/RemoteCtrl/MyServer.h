#pragma once
#include "MyThread.h"
#include "pch.h"
#include <map>
#include <MSWSock.h>
#include "Tool.h"
#include "MyQueue.h"

enum ServerOperator{
    MNone,
    MAccept,
    MRecv,
    MSend,
    MError
};

class CMyServer;
class MyClient;
typedef std::shared_ptr<MyClient> PCLIENT;


class CBaseOverlapped
{
public:
    DWORD m_operator;       // enum ServerOperator
    OVERLAPPED m_overlapped;
    std::vector<char> m_buffer;

    ThreadWorker m_worker;  // 处理函数
    CMyServer* m_server;    // 服务器
    MyClient* m_pClient;    // 对应的客户端。之前的 PCLIENT ，存在自己引用自己情况，最终释放 map 里的 client 不会调用析构函数。
 
    WSABUF m_wsaBuffer;     //

    virtual ~CBaseOverlapped() {
        m_buffer.clear();
    }
};


template<ServerOperator op>
class AcceptOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped() {
        m_operator = MAccept;
        m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped::AcceptWorker);
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
        m_server = NULL;
        memset(&m_wsaBuffer, 0, sizeof(WSABUF));
    }

    int AcceptWorker() {
        // TODO:
        INT lLength = 0, rLength = 0;
        if (m_pClient->GetBufferSize() > 0) {       // m_pClient->GetBufferSize() > 0 意味着 之前 AceeptEx 接收到 client 信息
            LPSOCKADDR pLocalAddr, pRemoteAddr;
            GetAcceptExSockaddrs((PVOID)*m_pClient, 0,
                sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
                (sockaddr**)&pLocalAddr, &lLength,//本地地址
                (sockaddr**)&pRemoteAddr, &rLength//远程地址
            );

            memcpy(m_pClient->GetLocalAddr(), pLocalAddr, sizeof(sockaddr_in));
            memcpy(m_pClient->GetRemoteAddr(), pRemoteAddr, sizeof(sockaddr_in));

            // AcceptEx得到的客户端套接字 绑定 iocp
            m_server->BindNewSocket(*m_pClient);

            // WSARecv 通知 IOCP 相关消息。之后再去 recv。这里并没有使用到 WSABUF。
            int ret = WSARecv((SOCKET)*m_pClient,  m_pClient->RecvWSABuf(), 1, *m_pClient, &m_pClient->flags(), m_pClient->RecvOverlapped(), NULL);
            if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
                //TODO：报错
                TRACE("ret = %d error = %d\r\n", ret, GetLastError());
            }

            // 结束之后，再次Accept进行下一次连接。
            if (!m_server->NewAccept())
            {
                return -2;
            }
        }
        return -1;
    }
};
typedef AcceptOverlapped<MAccept> ACCEPTOVERLAPPED;


template<ServerOperator>
class RecvOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    RecvOverlapped() {
        m_operator = MRecv;
        m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped::RecvWorker);
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);

        memset(&m_wsaBuffer, 0, sizeof(WSABUF));
    }

    int RecvWorker() {
        int ret = m_pClient->Recv();
        return ret;
    }
};
typedef RecvOverlapped<MRecv> RECVOVERLAPPED;


template<ServerOperator>
class SendOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    SendOverlapped() {
        m_operator = MSend;
        m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped::SendWorker);
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
        memset(&m_wsaBuffer, 0, sizeof(WSABUF));
    }

    int SendWorker() {
        // TODO:
        /*
            1. Send可能不会立即完成。

        */

        return -1;
    }
};
typedef SendOverlapped<MSend> SENDOVERLAPPED;


template<ServerOperator>
class ErrorOverlapped : public CBaseOverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped(){
        m_operator = MError;
        m_worker = ThreadWorker(this, (FUNCTYPE)&ErrorOverlapped::ErrorWorker);
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
        memset(&m_wsaBuffer, 0, sizeof(WSABUF));
    }

    int ErrorWorker() {
        // TODO:
        return -1;
    }
};
typedef ErrorOverlapped<MError> ERROROVERLAPPED;

class MyClient :public ThreadFuncBase{
public:
    MyClient() :
        m_isBusy(false), m_flags(0), m_received(0), m_usedIndex(0),
        m_overlapped(new ACCEPTOVERLAPPED()),
        m_recvOverlapped(new RECVOVERLAPPED()),
        m_sendOverlapped(new SENDOVERLAPPED()),
        m_vecSend(this, (SENDCALLBACK)&MyClient::SendData)      // 用SENDCALLBACK，因为那边是关于 T 的别名，不知道 T 是多少，所以还得搞一个别名。
    {
        m_sock = WSASocketW(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        m_buffer.resize(1024);
        memset(&m_laddr, 0, sizeof(sockaddr_in));
        memset(&m_raddr, 0, sizeof(sockaddr_in));
    }

    ~MyClient() {
        closesocket(m_sock);
        m_overlapped.reset();
        m_recvOverlapped.reset();
        m_sendOverlapped.reset();
        m_buffer.clear();
        m_vecSend.Clear();
    }

    void SetOverlapped(PCLIENT& ptr) {
        // 使得 overlapped 可以访问对应的 m_pClient
        m_overlapped->m_pClient = ptr.get();
        m_recvOverlapped->m_pClient = ptr.get();
        m_sendOverlapped->m_pClient = ptr.get();
    }

    operator SOCKET() {
        return m_sock;
    }

    // 当你将对象转换为 PVOID 或 void* 类型时，会自动调用这个操作符来获取所需的指针值。
    operator PVOID() {
        return (PVOID)m_buffer.data();
    }

    operator LPDWORD() {
        return &m_received;
    }

    operator LPOVERLAPPED() {           // AcceptEx overlapped用。 Accept特殊的，通过强转获取对应的overlapped。recv，send用下面的方法返回。
        return &m_overlapped->m_overlapped;
    }
    
    LPWSABUF RecvWSABuf() {
        return &m_recvOverlapped->m_wsaBuffer;
    }

    LPWSAOVERLAPPED RecvOverlapped() {
        return &m_recvOverlapped->m_overlapped;
    }

    LPWSABUF SendWSABuf() {
        return &m_sendOverlapped->m_wsaBuffer;
    }

    LPWSAOVERLAPPED SendOverlapped() {
        return &m_sendOverlapped->m_overlapped;
    }

    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr; }
    size_t GetBufferSize() const { return m_buffer.size(); }
    DWORD& flags() { return m_flags; }

    int Recv() {
        int ret = recv(m_sock, m_buffer.data() + m_usedIndex, m_buffer.size() - m_usedIndex, 0);
        if (ret <= 0) {
            TRACE("error %d\r\n", GetLastError());
            return -1;
        }
        m_usedIndex += (size_t)ret;
        // TODO :解析数据
        CTool::Dump((BYTE*)m_buffer.data(), ret);
        return 0;
    }

    int Send(void* buffer, size_t nSize) {
        std::vector<char> data(nSize);
        memcpy(data.data(), buffer, nSize);
        if (m_vecSend.PushBack(data) == false) {
            return -1;
        }
        return 0;
    }

    int SendData(std::vector<char>& data) {
        // TODO：实际上从队列中取包，发数据。
        if (m_vecSend.Size() > 0) {
            int ret = WSASend(m_sock, SendWSABuf(), 1, &m_received, m_flags, &m_sendOverlapped->m_overlapped, NULL);
            if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
                CTool::ShowError();
                return -1;
            }
        }
        return 0; 
    }

private:
    SOCKET m_sock;
    DWORD m_received;
    DWORD m_flags;
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;  // 传出参数，为了得到操作，缓冲区，服务器信息。
    std::shared_ptr<RECVOVERLAPPED> m_recvOverlapped;
    std::shared_ptr<SENDOVERLAPPED> m_sendOverlapped;
    std::vector<char> m_buffer;     // 用于Accept
    size_t m_usedIndex;             // 已经使用了的缓冲区
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isBusy;
    
    CMySendQueue<std::vector<char>> m_vecSend;      // 发送数据队列
};

class CMyServer :
    public ThreadFuncBase
{
public:
    CMyServer(const std::string& IP = "0.0.0.0" , short Port = 9527): threadPool(10){
        m_hIocp = INVALID_HANDLE_VALUE;
        m_sock = INVALID_SOCKET;

        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(Port);
        m_addr.sin_addr.s_addr = inet_addr(IP.c_str());
    }

    ~CMyServer(){
        closesocket(m_sock);
        for (auto it = m_clients.begin(); it != m_clients.end(); it++) {
            it->second.reset();     // share_ptr 存在自己引用了自己情况，所以不会析构。
        }
        m_clients.clear();
        CloseHandle(m_hIocp);
        threadPool.Stop();
        WSACleanup();
    }

    bool StartService() {
        WSADATA WSAData;
        WSAStartup(MAKEWORD(2, 2), &WSAData);
        m_sock = WSASocketW(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        int opt = -1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        if (bind(m_sock, (SOCKADDR*)&m_addr, sizeof(sockaddr_in)) == SOCKET_ERROR) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }

        if (listen(m_sock, 5) == SOCKET_ERROR) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }

        m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        if (m_hIocp == NULL) {
            m_hIocp = INVALID_HANDLE_VALUE;
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIocp, (ULONG_PTR)this, 0);


        threadPool.Invoke();
        // 创建一个线程，专门用于GetQueuedCompletionStatus处理iocp消息。
        int ret = threadPool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CMyServer::threadIocp));
        TRACE("Thread index =  %d\r\n", ret);
        if (!NewAccept()) return false;
        return true;
    }

    bool NewAccept() {
        PCLIENT pClient(new MyClient());
        pClient->SetOverlapped(pClient);

        m_clients.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));        // ? 不理解

        // AcceptEx
        // 如果客户端在连接建立时发送了数据，这些数据会被存储在 lpOutputBuffer 缓冲区中。
        // 你可以通过 GetAcceptExSockaddrs 函数来获取客户端和服务器的地址信息
        // pClient 会传出参数
        TRACE("[服务器]%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
       
        // AcceptEx完成后，发生I/O完成包到IOCP中。通过GetQueuedCompletionStatus可以获取到对应的lpOverlapped（AcceptEx的最后一个参数），初始化的时候已经设置为了SENDOVERLAPPED；
        
        // 当您调用 AcceptEx 时，它会立即返回，并启动一个异步接受操作。
        // 如果此时已经有新的连接请求等待处理，AcceptEx 会立即开始处理该请求。
        // 如果没有新的连接请求，AcceptEx 会等待新的连接请求到达。
        
        // 写入 m_buffer 缓冲成员
        if (!AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient)) {
            TRACE("%d\r\n", WSAGetLastError());
            if ((WSAGetLastError() != WSA_IO_PENDING)) {
                closesocket(m_sock);
                m_hIocp = INVALID_HANDLE_VALUE;
                m_sock = INVALID_SOCKET;
                return false;
            } 
        }
        return true;
    }


    void BindNewSocket(SOCKET s) {
        if (CreateIoCompletionPort((HANDLE)s, m_hIocp, (ULONG_PTR)this, 0) == NULL) {
            TRACE("error %d \r\n", GetLastError());
        }
    }

private:
    int threadIocp() {
        LPOVERLAPPED lpOverlapped = NULL;
        DWORD transferred = 0;
        ULONG_PTR key = 0;
        TRACE("[服务器]%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        if(GetQueuedCompletionStatus(m_hIocp, &transferred, &key, &lpOverlapped, INFINITE))
        {  
            if (key != 0)
            {
                CBaseOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, CBaseOverlapped, m_overlapped);
                TRACE("pOverlapped->m_operator %d \r\n", pOverlapped->m_operator);

                pOverlapped->m_server = this;       // 添加指向server的指针，便于方法内调用 NewAccept()

                switch (pOverlapped->m_operator) {
                case MAccept:
                {
                    ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                    // eg: 多继承情况下，指针转换会存在偏差。
                    // ACCEPTOVERLAPPED 继承了 CBaseOverlapped 以及 ThreadFuncBase
                    // pOver 是 指向 CBaseOverlapped 开头的地址。
                    // CBaseOverlapped::m_worker 的初始化的时候（在AcceptOverlapped），会调用 ThreadWorker 初始化，会把 thiz(this) 转成 ThreadFuncBase，thiz 和 pOver相比就有偏移。

                    // 解决方法：	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f) {}

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
        return 0;
    }

private:
    CMyThreadPool threadPool;
    SOCKET m_sock;
    HANDLE m_hIocp;

    std::map<SOCKET, std::shared_ptr<MyClient>> m_clients;
    sockaddr_in m_addr;
};

