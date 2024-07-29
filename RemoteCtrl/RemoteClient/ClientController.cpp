#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;

CClientController* CClientController::m_instance = NULL;

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
		{WM_SEND_PACK, &CClientController::OnSendPack},
		{WM_SEND_PACK, &CClientController::OnSendData},
		{WM_SEND_PACK, &CClientController::OnShowStatus},
		{WM_SEND_PACK, &CClientController::OnShowWatcher},
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
	WaitForSingleObject(hEvent, -1);

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
	while (!m_isClose) {
		if (m_remoteDlg.isFull() == false) {
			int ret = SendCommandPacket(6);
			if (ret == 6) {
				if (GetImage(m_remoteDlg.GetImage()) == 0) {
					m_remoteDlg.setImageStatus(true);
				}
				else {
					TRACE("获取图片失败！\r\n");
				}
			}
		}
		else {
			Sleep(1);
		}
	}
}


void __stdcall CClientController::threadEntryForDownloadFile(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadForDownloadFile();
	_endthreadex(0);
}


void CClientController::threadForDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile == NULL) {
		AfxMessageBox("没有权限保存该文件，或者文件无法创建！！！");
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	TRACE("[客户端]%s\r\n", LPCSTR(m_strRemote));
	CClientSocket* pClient = CClientSocket::getInstance();
	do {
		// CString --> LPCSTR --> BYTE*
		int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
		if (ret < 0) {
			AfxMessageBox("执行下载命令失败！");
			TRACE("执行下载命令失败，ret = %d\r\n", ret);
			break;
		}

		long long nLength = *(long long*)pClient->GetPacket().strData.c_str(); //待下载的文件长度
		if (nLength == 0) {
			AfxMessageBox("文件长度为零，或者无法读取文件！！！");
			break;
		}

		m_statusDlg.download_process.SetRange(0, 100);
		long long nCount = 0;
		while (nCount < nLength)
		{
			double cur_portion = nCount * 100.0 / nLength;
			m_statusDlg.download_process.SetPos(cur_portion);

			CString temp;
			temp.Format(_T("命令执行中 进度 = %f %%"), cur_portion);
			m_statusDlg.m_info.SetWindowText(temp);

			ret = pClient->DealCommand();

			if (ret < 0) {
				AfxMessageBox("传输失败！！！");
				TRACE("传输失败！！！，ret = %d\r\n", ret);
				break;
			}

			size_t cur_size = pClient->GetPacket().strData.size();

			fwrite(pClient->GetPacket().strData.c_str(), 1, cur_size, pFile);

			nCount += cur_size;

		}
	} while (0);
	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！"), _T("完成"));
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

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket* pPacket = (CPacket*)wParam;
	return pClient->Send(*pPacket);
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	char* pData = (char*)wParam;
	return pClient->Send(pData, (int)lParam);
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
