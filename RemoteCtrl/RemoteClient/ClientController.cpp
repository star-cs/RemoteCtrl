#include "pch.h"
#include "ClientController.h"

CClientController* CClientController::m_instance = NULL;

CClientController::CHelper CClientController::m_helper;

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	return pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, "*",
		strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		NULL, &m_remoteDlg);

	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();

		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建!!!"));
			return -1;
		}

		SendCommandPacket(m_remoteDlg.GetSafeHwnd(), 4, false, 
			(BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);

		m_remoteDlg.BeginWaitCursor();		// 设置光标为等待状态。
		m_statusDlg.m_info.SetWindowText(_T("命令执行中..."));
		m_statusDlg.download_process.SetRange(0, 100);
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 1;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(TEXT("下载完成!!"), TEXT("完成"));
	TRACE("下载完成\r\n");
}

void CClientController::StartWatchScreen()
{
	m_isClose = false;

	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadEntryForWatchScreen, 0, this);
	m_watchDlg.DoModal();

	m_isClose = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

CClientController::CClientController() :
	//初始化指定父窗口
	m_statusDlg(&m_remoteDlg),
	m_watchDlg(&m_remoteDlg),
	m_hThread(INVALID_HANDLE_VALUE),
	m_nThreadID(-1),
	m_isClose(true)
{
	struct {
		UINT nMsg;
		MSGFUNC func;
	}data[] = {
		{WM_SEND_STATUS, &CClientController::OnShowStatus},
		{WM_SEND_WATCH, &CClientController::OnShowWatcher},
		{(UINT)-1, NULL},
	};

	for (int i = 0; data[i].func != NULL; i++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(data[i].nMsg, data[i].func));
	}
}

CClientController* CClientController::getInstance()
{
	static CClientController instance;
	return &instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::SendMessage(MSG msg)
{ 
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)return -2;

	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)&hEvent);
	//通过事件通知，并通过结构体存储结果。
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);

	return info.result;
}


void __stdcall CClientController::threadEntryForWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthreadex(0);
}


void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClose) {
		if (GetTickCount64() - nTick < 50)
		{
			Sleep(50 - DWORD(GetTickCount64() - nTick));
		}
		nTick = GetTickCount64();

		if (m_watchDlg.isFull() == false) {
			bool ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL , 0);
			if (ret == false) {
				TRACE("获取图片失败，屏幕监控命令失败\r\n");
			}
		}

		Sleep(1);
	}
}


unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}


void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_SEND_MESSAGE) {
			//为了获取到方法的返回值，用这个自定义消息以及消息。
			MSGINFO* pmsgInfo = (MSGINFO*)msg.wParam;
			HANDLE* pEvent = (HANDLE*)msg.lParam;

			auto it = m_mapFunc.find(pmsgInfo->msg.message);
			if (it != m_mapFunc.end()) {
				pmsgInfo->result = (this->*it->second)(pmsgInfo->msg.message, pmsgInfo->msg.wParam, pmsgInfo->msg.lParam);
			}
			else {
				pmsgInfo->result = -1;
			}
			SetEvent(pEvent);
		}
		else {
			auto it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}

	}
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}

void CClientController::setStatus(double cur_position)
{
	m_statusDlg.download_process.SetPos(cur_position);

	CString temp;
	temp.Format(_T("命令执行中 进度 = %f %%"), cur_position);

	m_statusDlg.m_info.SetWindowText(temp);
}
