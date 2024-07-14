// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"

#include "ServerSocket.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int GetDriverInfo() {
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0) {
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }
    
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    //Dump((BYTE*)&pack, pack.nLength + 6);       //FF FE CC CC 07 00 00 00 01 00 CC CC CC 出错（因为内存对齐）
    //Dump((BYTE*)&pack, pack.nLength + 6);       //FF FE 07 00 00 00 01 00 A0 31 11 B4 6F string数据传的是地址。
    Dump((BYTE*)pack.Data(), pack.Size());        //FF FE 07 00 00 00 01 00 43 2C 44 B3 00 

    //CServerSocket::getInstance()->Send(pack);
    return 0;
}



int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            // 套接字初始化
            //CServerSocket* pserver = CServerSocket::getInstance();
            //if (pserver->InitSocket() == false) {
            //    MessageBox(NULL, _T("网络初始化异常，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //}
            //int count = 0;
            //while (CServerSocket::getInstance() != NULL) { 
            //    if (pserver->AcceptClient() == false) {
            //        if (count >= 3) {
            //            MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO
            //}

            //文件操作
            GetDriverInfo();
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
