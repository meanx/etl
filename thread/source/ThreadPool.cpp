#include "ThreadPool.h"
#include <cassert>
#include <deque>
#include <vector>
using namespace std;


namespace threadm {


/*
** definition of class ThreadPoolImpl
*/
class ThreadPoolImpl {
public:
	 ThreadPoolImpl();
	~ThreadPoolImpl();

	bool Create(unsigned int nMinThread, unsigned int nMaxThread, unsigned int nMaxPendingTask);
	
	void Destroy();

	bool Execute(Runnable *pRunnable);

	void Statistic(unsigned int &nThread, unsigned int &nTask);

private:
	bool			 m_bCreate;
	unsigned int	 m_nMinThread;
	unsigned int	 m_nMaxThread;
	unsigned int	 m_nMaxPendingTask;
	TMutex			 m_mtxThread;
	TMutex			 m_mtxThreadDead;
	TMutex			 m_mtxTasks;
	vector<Thread*>  m_threads;
	vector<Thread*>  m_threadDead;
	deque<Runnable*> m_tasks;
	typedef vector<Thread*>::iterator  ThreadIterator;
	typedef deque<Runnable*>::iterator TasksIterator;

private:
	class Worker : public Runnable {
	public:
		Worker(ThreadPoolImpl *pPoolImpl, Runnable *pRunnable = 0);
		virtual ~Worker();

		virtual void Run();
		virtual void Stop();

	private:
		ThreadPoolImpl *m_pPoolImpl;
		Runnable	   *m_pRunnable;
		threadm::TAtom  m_bRun;
	};
};

ThreadPoolImpl::Worker::Worker(ThreadPoolImpl *pPoolImpl, Runnable *pRunnable/* = 0*/)
: m_pPoolImpl(pPoolImpl)
, m_pRunnable(pRunnable)
, m_bRun(1)
{

}

ThreadPoolImpl::Worker::~Worker()
{
// 	Stop(); // before delete Worker, the call thread must call pWorker->Stop();
	m_pPoolImpl = 0;
	if (0 != m_pRunnable) {
		delete m_pRunnable;
		m_pRunnable = 0;
	}
}

void ThreadPoolImpl::Worker::Run()
{
	Runnable *pTask = 0;

	while (m_bRun.Read())
	{
		if (0 == m_pRunnable) {
			TAutoLock lock(m_pPoolImpl->m_mtxTasks);
			if (!m_pPoolImpl->m_tasks.empty()) {
				pTask = m_pPoolImpl->m_tasks.front();
				m_pPoolImpl->m_tasks.pop_front();
			}
		} else {
			pTask = m_pRunnable;
			m_pRunnable = 0;
		}

		if (0 == pTask)
		{
			if (m_pPoolImpl->m_threads.size() > m_pPoolImpl->m_nMinThread)
			{
				this->Stop();

				m_pPoolImpl->m_mtxThread.Lock();

				ThreadIterator itr = m_pPoolImpl->m_threads.begin();
				while (itr != m_pPoolImpl->m_threads.end()) {
					if (this == (*itr)->GetRunnable()) {
						break;
					}
					++itr;
				}
				if (itr != m_pPoolImpl->m_threads.end()) {
					Thread *pThread = (*itr);
					m_pPoolImpl->m_threads.erase(itr);
					TAutoLock lock(m_pPoolImpl->m_mtxThreadDead);
					m_pPoolImpl->m_threadDead.push_back(pThread);
				}

				m_pPoolImpl->m_mtxThread.Unlock();
			}
			else
			{
				m_pPoolImpl->m_mtxThreadDead.Lock();
				if (!m_pPoolImpl->m_threadDead.empty())
				{
					ThreadIterator itr = m_pPoolImpl->m_threadDead.begin();
					while (itr != m_pPoolImpl->m_threadDead.end())
					{
						(*itr)->Join();
						delete (*itr);
						(*itr) = 0;
						m_pPoolImpl->m_threadDead.erase(itr);
						itr = m_pPoolImpl->m_threadDead.begin();
					}
					m_pPoolImpl->m_mtxThreadDead.Unlock();
				}
				else 
				{
					m_pPoolImpl->m_mtxThreadDead.Unlock();
					Thread::Sleep(50); // to be continue...
				}
			}
		}
		else
		{
			pTask->Run();
			delete pTask;
			pTask = 0;
		}

	} //~while
}

void ThreadPoolImpl::Worker::Stop()
{
	if (0 != m_pRunnable) {
		m_pRunnable->Stop();
	}
	m_bRun.Set(0);
}

ThreadPoolImpl::ThreadPoolImpl()
: m_bCreate(false), m_nMinThread(0), m_nMaxThread(0), m_nMaxPendingTask(0)
{

}

ThreadPoolImpl::~ThreadPoolImpl()
{
	Destroy();
}

bool ThreadPoolImpl::Create(unsigned int nMinThread, unsigned int nMaxThread, unsigned int nMaxPendingTask)
{
	if (m_bCreate) {	// disallow recreate operation.
		return true;	// discard the recreate parameters (i.e. nMin, nMax, nMax)
	}

	if (0 == nMinThread || nMinThread > nMaxThread) {
		return false;
	}

	m_nMinThread = nMinThread;
	m_nMaxThread = nMaxThread;
	m_nMaxPendingTask = nMaxPendingTask;

	unsigned int i = 0;
	for (; i < m_nMinThread; ++i)
	{
		Thread *pThread = new Thread(new ThreadPoolImpl::Worker(this, 0));
		if (0 == pThread) {
			break;
		}
		TAutoLock lock(m_mtxThread);
		m_threads.push_back(pThread);
		pThread->Start();
	}

	m_bCreate = i > 0 ? true : false;
	return m_bCreate;
}

void ThreadPoolImpl::Destroy()
{
	if (!m_bCreate) {
		return;
	}

	do
	{
		//!should wait dead thread have been deleted.
		unsigned int nThread = 0, nTask = 0;
		this->Statistic(nThread, nTask);
		if (m_nMinThread == nThread && 0 == nTask) {
			break;
		}
		else {
			Thread::Sleep(10);
		}

	} while (1);

	TAutoLock lock(m_mtxThread);
	ThreadIterator itr = m_threads.begin();
	while (itr != m_threads.end())
	{
		//!should wait all thread have been done.
		Thread   *pThread = (*itr);
		Runnable *pWorker = pThread->GetRunnable();
		pWorker->Stop();
		pThread->Join();
		delete pThread;
		(*itr) = 0;
		m_threads.erase(itr);
		itr = m_threads.begin();
	}

	//!now, there should be no task.
	assert(m_tasks.empty());
	m_bCreate = false;
}

bool ThreadPoolImpl::Execute(Runnable *pRunnable)
{
	if (0 == pRunnable || !m_bCreate) {
		return false;
	}

	bool bRet = false;
	if (m_tasks.size() >= m_nMaxPendingTask)
	{
		if (m_threads.size() < m_nMaxThread) {
			Thread *pThread = new Thread(new ThreadPoolImpl::Worker(this, pRunnable));
			if (0 != pThread) {
				TAutoLock lock(m_mtxThread);
				m_threads.push_back(pThread);
				pThread->Start();
				bRet = true;
			}
		}
		else {
			// abandon the task.
			bRet = false;
		}
	}
	else
	{
		TAutoLock lock(m_mtxTasks);
		m_tasks.push_back(pRunnable);
		bRet = true;
	}

	return bRet;
}

void ThreadPoolImpl::Statistic(unsigned int &nThread, unsigned int &nTask)
{
	nThread = (unsigned int)m_threads.size();
	nTask   = (unsigned int)m_tasks.size();
}

/*
** implement of ThreadPool
*/
ThreadPool::ThreadPool() : m_pPoolImpl(new ThreadPoolImpl())
{
	assert(0 != m_pPoolImpl);
}

ThreadPool::~ThreadPool()
{
	Destroy();

	delete m_pPoolImpl;
	m_pPoolImpl = 0;
}

bool ThreadPool::Create(unsigned int nMinThread, unsigned int nMaxThread, unsigned int nMaxPendingTask)
{
	return m_pPoolImpl->Create(nMinThread, nMaxThread, nMaxPendingTask);
}

void ThreadPool::Destroy()
{
	m_pPoolImpl->Destroy();
}

bool ThreadPool::Execute(Runnable *pRunnable)
{
	return m_pPoolImpl->Execute(pRunnable);
}

void ThreadPool::Statistic(unsigned int &nThread, unsigned int &nTask)
{
	return m_pPoolImpl->Statistic(nThread, nTask);
}


} //~namespace threadm