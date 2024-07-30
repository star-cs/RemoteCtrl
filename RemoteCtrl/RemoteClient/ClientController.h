#pragma once

#include "ClientSocket.h"

#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "WatchDlg.h"
#include <map>
#include "resource.h"
#include "Tool.h"

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


	void UpdataAddress(int nIP, int nPort) {
		CClientSocket::getInstance()->UpdataAddress(nIP, nPort);
	}

	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}

	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}


	// 1 查看磁盘分区
	// 2 查看指定目录下的文件
	// 3 打开文件
	// 4 下载文件（服务端文件传给客户端）
	// 9 删除文件
	// 5 鼠标操作
	// 6 发送屏幕内容
	// 7 锁机
	// 8 解锁
	// 2024 测试连接
	// 返回cmd，失败返回-1。
	// 默认，只接收一次数据就关闭连接。
	int SendCommandPacket(
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		std::list<CPacket>* recvPackets = NULL) {

		CClientSocket* pClient = CClientSocket::getInstance();

		HANDLE hEvnet = CreateEvent(NULL, TRUE, FALSE, NULL);
		
		std::list<CPacket> lstPackets;
		if (recvPackets == NULL) {
			recvPackets = &lstPackets;
		}

		pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvnet), *recvPackets);
		
		if (recvPackets->size() > 0) {
			return recvPackets->front().sCmd;
		}

		return -1;
	}

	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTool::Byte2Image(image, pClient->GetPacket().strData); 
	}

	int DownFile(CString strPath);
	
	void StartWatchScreen();

protected:
	CClientController();

	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}

	static void __stdcall threadEntryForWatchScreen(void* arg);
	void threadWatchScreen();

	static void __stdcall threadEntryForDownloadFile(void* arg);
	void threadForDownloadFile();

	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();

	static void releaseInstance() {
		if (m_instance != nullptr) {
			CClientController* temp = m_instance;
			m_instance = nullptr;
			delete temp;
		}
	}

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
	unsigned int m_nThreadID;
	HANDLE m_hThreadDown;
	HANDLE m_hThreadWatch;

	// 下载文件的远程路径
	CString m_strRemote;
	// 下载文件的本地保存路径
	CString m_strLocal;

	bool m_isClose;	// 监控线程是否关闭

	static CClientController* m_instance;

	class CHelper {
	public:
		CHelper(){}

		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

