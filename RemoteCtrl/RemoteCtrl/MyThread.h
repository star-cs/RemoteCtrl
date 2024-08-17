#pragma once

#include "pch.h"
#include <vector>
#include <mutex>
#include <atomic>

class ThreadFuncBase {};

typedef int(ThreadFuncBase::* FUNCTYPE)();

class ThreadWorker {
public:
	ThreadWorker() :thiz(NULL), func(NULL) {}
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) :thiz(obj), func(f) {}

	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}

	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	bool IsVaild() const{
		return (thiz != NULL) && (func != NULL);
	}

	int operator()() {
		if (IsVaild()) {
			return (thiz->*func)();
		}
		return -1;
	}

public:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};


class CMyThread
{
public:
	CMyThread() : m_hThread(NULL), m_bStatus(false) {}

	~CMyThread() {
		Stop();
	}

	bool IsVaild() {
		if (m_hThread == NULL || m_hThread == INVALID_HANDLE_VALUE) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CMyThread::threadEntry, 0, this);
		if (!IsVaild()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;

		bool ret =  WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;

		UpdateWorker();

		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if(!worker.IsVaild()){
			m_worker.store(NULL);
			return;
		}
		if (m_worker.load() != NULL) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}

	// true 忙碌     false 空闲
	bool IsBusy() {
		return m_worker.load()->IsVaild();  
	}

private:
	static void threadEntry(void* arg) {
		CMyThread* thiz = (CMyThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}

	void ThreadWorker() {
		while (m_bStatus) {
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsVaild()) {
				int ret = worker();
				if (ret != 0) {
					CString str;
					str.Format(_T("thread found warning code %d \r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0) {
					m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
		
	}

private:
	HANDLE m_hThread;
	bool m_bStatus;		// false表示线程关闭，true表示线程运行

	std::atomic<::ThreadWorker*> m_worker;
};

class CMyThreadPool {
public:
	CMyThreadPool(){
		
	}

	CMyThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new CMyThread();
		}
	}

	~CMyThreadPool(){
		Stop();
		m_threads.clear();
	}

	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				ret = false;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	 
	bool Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
		return true;
	}

	// 返回-1 表示分配失败，所有的线程都在忙；大于等于0，表示第n个线程分配来处理。
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_lock.lock();
		// TODO: 可以改成空闲队列和忙碌队列
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (!m_threads[i]->IsBusy()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsVaild();
		}
		return false;
	}

private:
	std::mutex m_lock;
	std::vector<CMyThread*> m_threads;
};