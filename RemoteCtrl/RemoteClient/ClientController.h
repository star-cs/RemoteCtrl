#pragma once

#include "ClientSocket.h"

#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "WatchDlg.h"
#include <map>
#include "resource.h"

#define WM_SEND_PACK (WM_USER + 1)	//发送包数据
#define WM_SEND_DATA (WM_USER+2)	//发送数据
#define WM_SEND_STATUS (WM_USER+3)	//显示状态
#define WM_SEND_WATCH (WM_USER+4)	//远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) //自定义消息处理

class CClientController
{
public:
	static CClientController* getInstance();

	//初始化操作
	int InitController();

	//启动
	int Invoke(CWnd*& pMainWnd);

	LRESULT SendMessage(MSG msg);
protected:
	CClientController();

	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}

	void threadFunc();

	static unsigned __stdcall threadEntry(void* arg);

	static void releaseInstance() {
		if (m_instance != nullptr) {
			delete m_instance;
			m_instance = nullptr;
		}
	}

	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	typedef struct MsgInfo {
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	}MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lPAram);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	
	CWatchDlg m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;

	HANDLE m_hThread;
	unsigned m_nThreadID;

	static CClientController* m_instance;

	class CHelper {
	public:
		CHelper() {
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

