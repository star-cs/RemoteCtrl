// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "Tool.h"

#include "conio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP  2

enum {
    IocpListEmpty,
    IocpListPush,
    IocpListPop
};

typedef struct IocpParam {
    int nOperator;
    std::string strData;

    _beginthread_proc_type cbFunc;    //回调
    
    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
        nOperator = op; 
        strData = sData;
        cbFunc = cb;
    }

    IocpParam() {
        nOperator = -1;
    }
}IOCP_PARAM;


void threadMain(HANDLE hIocp)
{

    std::list<std::string> lstStrings;
    DWORD dwTransferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* pOverlapped = NULL;

    while (GetQueuedCompletionStatus(hIocp, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
        if ((dwTransferred == 0) && (CompletionKey == NULL)) {
            printf("thread is prepare to exit \r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOperator == IocpListPush) {
            lstStrings.push_back(pParam->strData);
        }
        else if (pParam->nOperator == IocpListPop) {
            std::string* pStr = NULL;
            if (lstStrings.size() > 0) {
                pStr = new std::string(lstStrings.front());
                lstStrings.pop_front();
            }
            if (pParam->cbFunc) {
                pParam->cbFunc(pStr);
            }
        }
        else if (pParam->nOperator == IocpListEmpty) {
            lstStrings.clear();
        }

        delete pParam;
    }
    lstStrings.clear();
}

void threadQueueKey(HANDLE hIocp)
{
    threadMain(hIocp); 
    _endthread(); // _endthread 和 _endthreadex 显示调用会导致在线程挂起的 C++ 析构函数不调用。所以需要另外调用功能函数实现。

}

void func(void* arg)
{
    std::string* pstr = (std::string*)arg;
    if (pstr != NULL) {
        printf("pop from list:%s\r\n", pstr->c_str());
        delete pstr;
    }
    else {
        printf("list is empty\r\n");
    }
}

int main()
{
    if (!CTool::Init()) return 1;
    printf("press any key to continue ... \r\n");

    HANDLE hIOCP = INVALID_HANDLE_VALUE;
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
    HANDLE hThread = (HANDLE)_beginthread(threadQueueKey, 0, hIOCP);

    ULONGLONG tick = GetTickCount64();
    ULONGLONG tick0 = GetTickCount64();

    while (_kbhit() == 0) {
        if (GetTickCount64() - tick0 > 1300) {
            PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "", func), NULL);
            tick0 = GetTickCount64();
        }

        if (GetTickCount64() - tick > 2000){
            PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "Hello World"), NULL);
            tick = GetTickCount64();
        }
        Sleep(1);
    }

    if (hIOCP != NULL) {
        PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
        WaitForSingleObject(hThread, INFINITE);
 
    }

    CloseHandle(hIOCP);
    /**
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
    */
    

    return 0;
}
