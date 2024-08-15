#pragma once
#include <list>
#include <atomic>

template <class T>
class CMyQueue
{
public:
	// 类内局部变量，用enum。
	enum {
		EQNone,
		EQPush,//入队
		EQPop,//出队
		EQSize,//大小
		EQClear//清空
	};

	typedef void(*PFUNC)(void*);
	typedef struct IocpParam {
		size_t nOperator;	//操作
		T Data;				//数据
		HANDLE hEvent;		//pop操作调用的事件。
		IocpParam(int op, const T& data, HANDLE hEve = NULL)
		{
			nOperator = op;
			Data = data;
			hEve = hEve;
		}
		IocpParam() {
			nOperator = EQNone;
		}
	}PPARAM; // Post Param 用于投递消息的结构体

public:
	CMyQueue() {
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = (HANDLE)_beginthread(threadEntry, 0, m_hCompeletionPort);

	}

	~CMyQueue() {
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}

	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(EQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		}
		BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) {
			delete pParam;
		}
		return ret;
	}

	bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQPop, data, hEvent);

		if (m_lock) {
			CloseHandle(hEvent); 
			return false;
		}

		BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}

		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;	// 事件触发为WAIT_OBJECT_0
		if (ret) {
			data = pParam.Data;
		}
		return ret;
	}

	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQSize, NULL, hEvent);

		if (m_lock) {
			CloseHandle(hEvent);
			return 0;
		}
		BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}

		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;	// 事件触发为WAIT_OBJECT_0
		if (ret) {
			return pParam.nOperator;
		}
		return -1;
	}

	bool Clear() {
		IocpParam* pParam = new IocpParam(EQClear, NULL);
		if (m_lock) {
			delete pParam;
			return false;
		}

		BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
		if (ret == false) {
			delete pParam;
		}
		return ret;
	}

private:
	static void threadEntry(void* arg) {
		CMyQueue* thiz = (CMyQueue*)arg;
		thiz->threadMain();
		_endthread();
	}

	void threadMain() {
		PPARAM* pParam = NULL;
		DWORD dwTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped;
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
		{
			if ((dwTransferred == 0) && (CompletionKey == NULL)) {
				printf("thread is prepare to exit \r\n");
				break;
			}

			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}


		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) {
			if ((dwTransferred == 0) && (CompletionKey == NULL)) {
				printf("thread is prepare to exit \r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}

		// 关闭完全端口映射
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}

	void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator)
		{
		case EQPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;

		case EQPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;

		case EQSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;

		case EQClear:
			m_lstData.clear();
			delete pParam;
			break;

		default:
			OutputDebugString("unlnown operator! \r\n");
			break;
		}
	}


private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;

	std::atomic<bool> m_lock;	// 队列是否正在析构

};

