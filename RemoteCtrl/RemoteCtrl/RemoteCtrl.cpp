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

int MakeDriverInfo() {
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

#include <io.h>
#include <list>

typedef struct file_info {
    file_info() {
        IsInvalid = FALSE;
        IsDirectory = -1;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL IsInvalid;         //是否有效
    BOOL IsDirectory;       //是否为目录 0否 1是
    BOOL HasNext;           //是否还有后续 0没有 1有
    char szFileName[256];   //文件名 
}FILEINFO, *PFILEINFO;


int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> listFileInfos;    考虑到大量文件情况，等待时间太久。
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！"));
        return -1;
    }

    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        //listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("发送失败"));
            return -4;
        }
        OutputDebugString(_T("没有权限，访问目录！"));
        return -2;
    }

    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件！"));
        return -3;
    }

    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        //finfo->IsInvalid = FALSE;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        if (CServerSocket::getInstance()->Send(pack) == false) {
            OutputDebugString(_T("发送失败"));
            return -4;
        }

    } while (!_findnext(hfind, &fdata)); 

    //空文件信息，标记结尾。
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    if (CServerSocket::getInstance()->Send(pack) == false) {
        OutputDebugString(_T("发送失败"));
        return -4;
    }

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
 
            int nCmd = 1;
            switch (nCmd) {
            case 1:
                //查看磁盘分区
                MakeDriverInfo();
                break;
            case 2:
                MakeDirectoryInfo();
                break;
                 
            }
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
