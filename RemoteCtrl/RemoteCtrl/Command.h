#pragma once

#include <atlimage.h>
#include <direct.h>

#include <map>

#include <io.h>
#include <list>
#include "LockInfoDialog.h"
#include "resource.h"

#include "Tool.h"
#include "Packet.h"

class CCommand
{
public:
	CCommand();

	~CCommand(){
	}

    static void RunCommand(void* arg, int status, std::list<CPacket>& listPackets, CPacket& inPacket) {
        CCommand* thiz = (CCommand*)arg;
        if (status > 0) {
            int ret = thiz->ExcuteCommand(status, listPackets, inPacket);
            if (ret != 0) { 
                TRACE("[服务器]执行命令失败，%d ret=%d\r\n", status, ret);
            }
        }
        else {
            MessageBox(NULL, _T("无法正常接入用户，自动重试！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
        }
    }


    int ExcuteCommand(int nCmd, std::list<CPacket>& listPackets, CPacket& inPacket);

protected:

	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& listPackets, CPacket& inPacket);	//成员函数指针
	
    std::map<int, CMDFUNC> m_mapFunction;
    CLockInfoDialog dlg;

    unsigned threadId;


protected:

    static unsigned __stdcall threadEntryForLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadForLockDlg();
        _endthreadex(0);
        return 0;
    }

    void threadForLockDlg() {
        TRACE("[服务器]%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        //非模态
        dlg.Create(IDD_DIALOG_INFO, NULL);
         
        dlg.ShowWindow(SW_SHOW);

        //全屏遮蔽
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);

        rect.bottom = LONG(rect.bottom * 1.08);

        dlg.MoveWindow(rect);

        // 提示
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWidth = rtText.Width() / 2;
            int nHeight = rtText.Height() / 2;
            int x = (rect.right - nWidth) / 2;
            int y = (rect.bottom - nHeight) / 2;

            pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
            pText->SetWindowText(_T("请联系管理员解锁"));
        }

        //窗口置顶
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

        //限制鼠标功能
        ShowCursor(false);

        //隐藏任务栏
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);

        //限制鼠标活动范围
        ClipCursor(rect);

        MSG msg;

        //消息和线程相绑定。
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) {
                TRACE("[服务器]msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x41) { //Esc（1B） A（0x41）
                    break;
                }
            }
        }


        ClipCursor(NULL);
        //恢复鼠标
        ShowCursor(true);
        //恢复任务栏
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);

        //少了这个没反应 
        dlg.DestroyWindow();
    }

    int MakeDriverInfo(std::list<CPacket>& listPackets, CPacket& inPacket) {
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0) {
                result += 'A' + i - 1;
                result += ',';
            }
        }

        listPackets.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));

        return 0;
    }

    int MakeDirectoryInfo(std::list<CPacket>& listPackets, CPacket& inPacket) {

        std::string strPath = inPacket.strData;

        if (_chdir(strPath.c_str()) != 0) {
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            
            listPackets.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
           
            OutputDebugString(_T("没有权限，访问目录！"));
            return -2;
        }

        _finddata_t fdata;
        long long hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            OutputDebugString(_T("没有找到任何文件！"));

            FILEINFO finfo;
            finfo.HasNext = FALSE;

            listPackets.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

            return -3;
        }
        int count = 0;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            TRACE("[服务器]%s HasNext=%d IsDirectory=%d IsInvalid=%d \r\n", finfo.szFileName, finfo.HasNext, finfo.IsDirectory, finfo.IsInvalid);
             
            listPackets.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

            count++;

        } while (!_findnext(hfind, &fdata));
        TRACE("[服务器]file_count = %d\n", count);

        //空文件信息，标记结尾。
        FILEINFO finfo;
        finfo.HasNext = FALSE;
        listPackets.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

        return 0;
    }

    int RunFile(std::list<CPacket>& listPackets, CPacket& inPacket) {

        std::string strPath = inPacket.strData;
    
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

        listPackets.push_back(CPacket(3, NULL, 0));

        return 0;
    }

    //服务器传给客户端 文件。
    int DownloadFile(std::list<CPacket>& listPackets, CPacket& inPacket) {
        std::string strPath = inPacket.strData;

        long long data = 0;     //文件长度
        //FILE* pFile = fopen(strPath.c_str(), "rb");          
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");   //解决 _WINSOCK_DEPRECATED_NO_WARNINGS 问题
        if (err != 0) {
            listPackets.push_back(CPacket(4, (BYTE*)&data, 0));
           
            return -1;
        }

        if (pFile != NULL) {
            //发送文件大小信息，用于计算下载文件的进度信息。
            fseek(pFile, 0, SEEK_END);
            data = ftell(pFile);
            listPackets.push_back(CPacket(4, (BYTE*)&data, 8));

            fseek(pFile, 0, SEEK_SET);

            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                //如果文件大小是1024的整数倍那么，就会发出去一个空包结尾。
                if (rlen > 0) {
                    listPackets.push_back(CPacket(4, (BYTE*)&buffer, rlen));
                }
                    
            } while (rlen >= 1024);

            fclose(pFile);
        }
        listPackets.push_back(CPacket(4, NULL, 0));

        return 0;
    }

    int MouseEvent(std::list<CPacket>& listPackets, CPacket& inPacket) {
        MOUSEEV mouse;

        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

        WORD nFlags = 0;

        //确保不在switch里添加if判断语句
        switch (mouse.nButton)
        {
        case 0:     //左键
            nFlags = 1;
            break;
        case 1:     //右键
            nFlags = 2;
            break;
        case 2:     //中键
            nFlags = 4;
            break;
        case 3:     //没有按键
            nFlags = 8;
            break;
        }

        // 鼠标移动用mouse_event
        SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);


        switch (mouse.nAction)
        {
        case 0: //点击
            nFlags |= 0x10;
            break;
        case 1: //双击
            nFlags |= 0x20;
            break;
        case 2: //按下
            nFlags |= 0x40;
            break;
        case 3: //放开
            nFlags |= 0x80;
            break;
        case 4:
            break;
        default:
            break;
        }

        switch (nFlags)
        {
        case 0x11://左键点击
            mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x12://右键点击
            mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x14://中键点击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

            //case 0x08://仅鼠标移动
            //    mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            //    break;
        }

        listPackets.push_back(CPacket(5, NULL, 0));

        return 0;
    }

    int SendScreen(std::list<CPacket>& listPackets, CPacket& inPacket)
    {
        CImage screen;  //GDI

        HDC hScreen = ::GetDC(NULL);
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeigth = GetDeviceCaps(hScreen, VERTRES);

        screen.Create(nWidth, nHeigth, nBitPerPixel);

        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeigth, hScreen, 0, 0, SRCCOPY);

        ReleaseDC(NULL, hScreen);

        //screen.Save(_T("test2021.png"), Gdiplus::ImageFormatPNG);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL) {
            return -1;
        }

        IStream* pStream = NULL;

        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);

        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);

            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);

            PBYTE pData = (PBYTE)GlobalLock(hMem);

            SIZE_T nSize = GlobalSize(hMem);

            listPackets.push_back(CPacket(6, pData, nSize));

            GlobalUnlock(hMem);
        }

        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();

        return 0;
    }

    //添加一个 Dialog，样式Popup， 系统菜单False，边框None
    //创建线程，方便系统响应解锁消息。
    int LockMachine(std::list<CPacket>& listPackets, CPacket& inPacket)
    {
        if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
            _beginthreadex(NULL, 0, &CCommand::threadEntryForLockDlg, this, 0, &threadId);
            TRACE("[服务器]%s(%d):%d\r\n", __FUNCTION__, __LINE__, threadId);
        }

        listPackets.push_back(CPacket(7, NULL, 0));

        return 0;
    }

    int UnlockMachine(std::list<CPacket>& listPackets, CPacket& inPacket)
    {
        //dlg.SendMessage(WM_KEYDOWN, 0x41, 001E0001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 001E0001);
        //这里是 winapi，从main创建一个线程。需要 向特定的线程 发消息。
        PostThreadMessage(threadId, WM_KEYDOWN, 0x41, 001E0001);

        listPackets.push_back(CPacket(8, NULL, 0));

        return 0;
    }

    int DeleteLocalFile(std::list<CPacket>& listPackets, CPacket& inPacket)
    {
        std::string filePath = inPacket.strData;

        DeleteFile(filePath.c_str());

        listPackets.push_back(CPacket(9, NULL, 0));

        return 0;
    }

    int TestConnect(std::list<CPacket>& listPackets, CPacket& inPacket)
    {
        listPackets.push_back(CPacket(2024, NULL, 0));
        return 0;
    }

};

