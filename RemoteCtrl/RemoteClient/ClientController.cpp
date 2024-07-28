#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;

CClientController* CClientController::m_instance = NULL;

CClientController::CClientController() :
	//初始化指定父窗口
	m_statusDlg(&m_remoteDlg),
	m_watchDlg(&m_remoteDlg),
	m_hThread(INVALID_HANDLE_VALUE),
	m_nThreadID(-1)
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
	if (m_instance == nullptr) {
		m_instance = new CClientController();
	}
	return m_instance;
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

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_SEND_MESSAGE) {
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

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
