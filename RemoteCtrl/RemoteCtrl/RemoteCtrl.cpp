// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "Tool.h"

#include "conio.h"
#include "MyQueue.h"

#include <MSWSock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "MyServer.h"
#include "MSocket.h"
#include "MNetwork.h"

// 唯一的应用程序对象

//系统路径
#define SYSTEMPATH TEXT("C:\\Windows\\SysWOW64\\RemoteCtrl.exe") //注意这里32位(SysWOW64)和64位(system32)默认系统变量位置不同
  
//开机自启文件路径
#define STARTUPPATH TEXT("C:\\Users\\root\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;


//选择开机启动方式
bool ChooseAutoInvoke() {
    CString strInfo = TEXT("启动远程监控！\n");
    strInfo += TEXT("继续运行该程序，将使得这台机器处于被监控状态!\n");
    strInfo += TEXT("按下“取消”按钮，退出程序!\n");

    int ret = MessageBox(NULL, strInfo, TEXT("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        if (!CTool::WriteStartupDir(STARTUPPATH))//尝试写入开机启动文件  设置开机自启
        {
        }
        else if (!CTool::WriteRefisterTable(SYSTEMPATH))//尝试写入注册表自启动 设置开机自启
        {
        }
        else
        {
            MessageBox(NULL, TEXT("设置开机自启动失败?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL)
    {
        return false;
    }
    return true;
}

void iocp() {
    CMyServer server;
    server.StartService();
    getchar();
}


/*
在 recvfrom 中，sockaddr (client_addr) 用于存储客户端的地址信息。
在 sendto 中，sockaddr (server_addr) 用于指定服务器的地址信息。
*/

int RecvFromCB(void* arg, MBuffer& buffer, MSockaddrIn& addr) {
    MServer* server = (MServer*)arg;
    server->SendTo(buffer, addr);
    return 0;
}

int SendToCB(void* arg, const MSockaddrIn& addr, int ret) {
    MServer* server = (MServer*)arg;
    printf("sendto done! %p \r\n", server);
    return 0;
}



void udp_server() {
    printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }

    std::list<sockaddr_in> lstClientAddr;
    sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(20000);

    if (-1 == bind(sock, (sockaddr*)&server_addr, sizeof(sockaddr))) {
        printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        closesocket(sock);
        return;
    }

    std::string buffer;
    buffer.resize(1024);
    int len = sizeof(client_addr);
    int ret = 0;

    while (!_kbhit()) {
        ret = recvfrom(sock, (char*)buffer.c_str(), buffer.size() - 1, 0, (sockaddr*)&client_addr, &len);
        if (ret > 0) {
            //CTool::Dump((BYTE*)buffer.c_str(), ret);

            if (lstClientAddr.size() == 0) {
                lstClientAddr.push_back(client_addr);

                printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client_addr.sin_addr.s_addr, ntohs(client_addr.sin_port));
                ret = sendto(sock, (char*)buffer.c_str(), sizeof(buffer), 0, (sockaddr*)&client_addr, len);
                printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            }
            else {
                memcpy((void*)buffer.c_str(), &lstClientAddr.front(), sizeof(sockaddr));
                ret = sendto(sock, (char*)buffer.c_str(), sizeof(buffer), 0, (sockaddr*)&client_addr, len);
                printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            }
        }
        else {
            printf("%s(%d):%s ERROR(%d) ret = %d \r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
        }
        Sleep(1);
    }

    closesocket(sock);
    printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);
}

void udp_client(bool isHost = true) {
    Sleep(2000);

    sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(20000);
    int len = sizeof(client_addr);
    SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }

    if (isHost == true) {
        printf("%s(%d):%s isHost\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "host client!\n";

        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server_addr, sizeof(sockaddr));
        printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
        if (ret > 0) {
            // recvfrom缓冲区不够，会失败。
            msg.resize(1024);
            memset((char*)msg.c_str(), 0, sizeof(msg));
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client_addr, &len);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            if (ret > 0) {
                printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client_addr.sin_addr.s_addr, ntohs(client_addr.sin_port));
                printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
            }

            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client_addr, &len);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
            if (ret > 0) {
                printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client_addr.sin_addr.s_addr, ntohs(client_addr.sin_port));
                printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
            }
        }
    }
    else {
        printf("%s(%d):%s isHost = %d\r\n", __FILE__, __LINE__, __FUNCTION__, isHost);
        std::string msg = "not host client!\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server_addr, sizeof(sockaddr));
        printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
        if (ret > 0) {
            msg.resize(1024);
            memset((char*)msg.c_str(), 0, sizeof(msg));
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client_addr, &len);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);

            sockaddr_in host_client;
            memcpy(&host_client, (char*)msg.c_str(), sizeof(msg));

            if (ret > 0) {
                printf("%s(%d):%s host-ip %08X host-port %d\r\n", __FILE__, __LINE__, __FUNCTION__, host_client.sin_addr.s_addr, ntohs(host_client.sin_port));
                printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());

                msg = "hello, i am client！";
                int ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&host_client, sizeof(sockaddr));
                printf("%s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, GetLastError(), ret);
            }
        }
    }
    closesocket(sock);
}


/*
1.udp服务器接收被控端发送来的key，用于绑定key和被控端的地址。
2.控制端发送key，查询被控端地址，并返回得到被控端地址
*/
void udp_server2() {
    printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);


    std::list<MSockaddrIn> lstClientAddr;

    std::string ip("127.0.0.1");
    MServerParamter param(
        ip, 20000, MTYPE::MTypeUDP, NULL, NULL, NULL, RecvFromCB, SendToCB
    );

    MServer server(param);
    server.Invoke(&server);
    printf("%s(%d):%s \r\n", __FILE__, __LINE__, __FUNCTION__);
    getchar();
    return;
}

void udp_client2(bool isHost = true) {
    Sleep(2000);

    MSockaddrIn to("127.0.0.1", 20000), from;

    MSOCKET sock(new MSocket(MTYPE::MTypeUDP));

    if (*sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return;
    }

    MBuffer buffer(1024);

    printf("%s(%d):%s isHost = %d\r\n", __FILE__, __LINE__, __FUNCTION__, isHost);

    if (isHost == true) {
        // udp情况下，
        // sCmd = 2025 表示 udp 打洞测试
        // strData = 1表示 被控端->服务器  2表示 控制端->服务器 3表示 控制端->被控端
        UdpHole u(1, 1234);
        CPacket pack(2025, (BYTE*)&u, sizeof(u));
        CTool::Dump((BYTE*)pack.Data(), pack.Size());

        int ret = sock->sendto(pack, to);
        printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
        if (ret > 0) {
            // recvfrom缓冲区不够，会失败。

            ret = sock->recvfrom(buffer, from);
            if (ret > 0) {
                printf("%s(%d):%s ip %s port %d\r\n", __FILE__, __LINE__, __FUNCTION__, from.GetIP().c_str(), from.GetPort());
            }

            ret = sock->recvfrom(buffer, from);
            if (ret > 0) {
                printf("%s(%d):%s ip %s port %d\r\n", __FILE__, __LINE__, __FUNCTION__, from.GetIP().c_str(), from.GetPort());
            }
        }
    }
    else {
       
        UdpHole u(2, 1234);
        CPacket pack(2025, (BYTE*)&u, sizeof(u));
        CTool::Dump((BYTE*)pack.Data(), pack.Size());

        int ret = sock->sendto(pack, to);
        printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
        if (ret > 0) {

            ret = sock->recvfrom(buffer, from);
            printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);

            if (ret > 0) {
                sockaddr_in host_client;
                size_t len = (size_t)ret;
                CPacket pack = CPacket((BYTE*)buffer, len);
                printf("xxx");
                CTool::Dump((BYTE*)pack.Data(), pack.Size());

                memcpy(&host_client, pack.strData.c_str(), sizeof(sockaddr_in));
                MSockaddrIn host = MSockaddrIn(host_client);

                printf("%s(%d):%s ip %s port %d\r\n", __FILE__, __LINE__, __FUNCTION__, host.GetIP().c_str(), host.GetPort());

                UdpHole u(3, 1234);
                CPacket pack2(2025, (BYTE*)&u, sizeof(u));
                CTool::Dump((BYTE*)pack2.Data(), pack2.Size());

                int ret = sock->sendto(pack2, host);
                printf("%s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, GetLastError(), ret);
            }
        }
    }
}


void InitSockEnv() {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
}

void ClearSockEnv() {
    WSACleanup();
}

int main(int argc, char* argv[])
{
    /*
    if (!CTool::Init()) return 1;
    
    InitSockEnv();

    if (argc == 1) {
        char wstrDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, wstrDir);

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));

        std::string stdCmd = argv[0];
        stdCmd += " 1";

        BOOL bRet = CreateProcessA(NULL, (LPSTR)stdCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
        if (bRet) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            TRACE("进程ID:%d\r\n", pi.dwProcessId);
            TRACE("线程ID:%d\r\n", pi.dwThreadId);
            stdCmd += " 2";

            bRet = CreateProcessA(NULL, (LPSTR)stdCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
            if (bRet) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                TRACE("进程ID:%d\r\n", pi.dwProcessId);
                TRACE("线程ID:%d\r\n", pi.dwThreadId);
                udp_server2();
            }
        }
    }
    else if (argc == 2) {   // 主客户端
        udp_client2();
    }
    else {                  // 从客户端
        udp_client2(false);
    }

     ClearSockEnv();
    */
   
    // iocp();

    //::exit(0);  终止程序，不会触发析构


    if (!CTool::IsAdmin()) {        //TODO:这里条件取反 为了测试方便避免提权操作
        if (!CTool::Init()) return 1;
        MessageBox(NULL, TEXT("管理员"), TEXT("用户状态"), 0);
        if (ChooseAutoInvoke()) {
            CCommand cmd;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {  
            case -1:
                MessageBox(NULL, _T("网络初始化异常，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;

            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            }
        }
    }
    else {  // 普通用户
        if (CTool::RunAsAdmin() == false) { //创建管理员进程重新启动exe程序
            CTool::ShowError();
            return -1;
        }
        MessageBox(NULL, TEXT("普通用户"), TEXT("用户状态"), 0);
    }
    
    return 0;
}
