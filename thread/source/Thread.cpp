/**
_WIN32_WINNT >= 0x0601	// Windows 7 
_WIN32_WINNT >= 0x0600	// Windows vista 
_WIN32_WINNT >= 0x0501	// Windows XP 
_WIN32_WINNT >= 0x0500	// Windows 2000 
_WIN32_WINNT >= 0x0400	// Windows NT4.0
*/
#include "Thread.h"
#include <cassert>
#include <string.h>		// strcpy

#if defined WIN32
#define _WIN32_WINNT 0x0501	// for TryEnterCriticalSection
#include <process.h>
#include <windows.h>
#elif defined _LINUX
#include <fcntl.h>      // For O_CREAT | O_EXCL | O_RDWR
#include <unistd.h>		// sleep/usleep
#include <pthread.h>
#include <semaphore.h>
#include <time.h>		// clock_gettime
#include <sys/time.h>
#endif


namespace threadm {


long AtomicSet(volatile long *pAtom, long i)
{
#if defined WIN32
	return InterlockedExchange((LPLONG)(pAtom), i);
#else defined _LINUX
	return __sync_lock_test_and_set(pAtom, i);
#endif
}
long AtomicAdd(volatile long *pAtom, long i)
{
#if defined WIN32
	return InterlockedExchangeAdd((LPLONG)(pAtom), i);
#else defined _LINUX
	return __sync_fetch_and_add(pAtom, i);
#endif
}
long AtomicSub(volatile long *pAtom, long i)
{
	return AtomicAdd(pAtom, -i);
}
long AtomicInc(volatile long *pAtom)
{
#if defined WIN32
	return InterlockedIncrement((LPLONG)(pAtom));
#else defined _LINUX
	return __sync_fetch_and_add(pAtom, 1);
#endif
}
long AtomicDec(volatile long *pAtom)
{
#if defined WIN32
	return InterlockedDecrement((LPLONG)(pAtom));
#else defined _LINUX
	return __sync_fetch_and_add(pAtom, -1);
#endif
}

class TMutexImpl {
public:
	TMutexImpl()
	{
#	if defined WIN32
		InitializeCriticalSection(&m_cs);
#	else defined _LINUX
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		int nRet = pthread_mutex_init(&m_mtx, &attr);
		assert(0 == nRet);
		pthread_mutexattr_destroy(&attr);
#	endif
	}

	~TMutexImpl()
	{
#	if defined WIN32
		DeleteCriticalSection(&m_cs);
#	else defined _LINUX
		int nRet = pthread_mutex_destroy(&m_mtx);
		assert(0 == nRet);
#	endif
	}

	void Lock()
	{
#	if defined WIN32
		EnterCriticalSection(&m_cs);
#	else defined _LINUX
		int nRet = pthread_mutex_lock(&m_mtx);
		assert(0 == nRet);
#	endif
	}

	bool TryLock()
	{
#	if defined WIN32
		if (!TryEnterCriticalSection(&m_cs)) {
			if (!TryEnterCriticalSection(&m_cs)) {
				return false ;
			}
		}
#	else defined _LINUX
		if (0 != pthread_mutex_trylock(&m_mtx)) {
			return false;
		}
#	endif
		
		return true ;
	}

	void Unlock()
	{
#	if defined WIN32
		LeaveCriticalSection(&m_cs);
#	else defined _LINUX
		int nRet = pthread_mutex_unlock(&m_mtx);
		assert(0 == nRet);
#	endif
	}

public:
	void* GetData() {
# if defined WIN32
		return &m_cs;
# else defined _LINUX
		return &m_mtx;
# endif
	}

private:
# if defined WIN32
	CRITICAL_SECTION m_cs;
# else defined _LINUX
	pthread_mutex_t  m_mtx;
# endif
};

TMutex::TMutex() : m_pMutexImp(new TMutexImpl())
{
	assert(NULL != m_pMutexImp);
}

TMutex::~TMutex()
{
	delete m_pMutexImp;
	m_pMutexImp = 0 ;
}

void TMutex::Lock()
{
	m_pMutexImp->Lock();
}

bool TMutex::TryLock()
{
	return m_pMutexImp->TryLock();
}

void TMutex::Unlock()
{
	m_pMutexImp->Unlock();
}

class TEventImpl
{
public:
	TEventImpl()
	{
#	if defined WIN32
		m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
#	elif defined _LINUX
		int nRet = pthread_cond_init(&m_event, NULL);
		assert(0 == nRet);
#	endif
	}
	virtual ~TEventImpl()
	{
#	if defined WIN32
		CloseHandle(m_event); m_event = NULL; // CloseHandle(NULL) will not cause error.
#	elif defined _LINUX
		int nRet = pthread_cond_destroy(&m_event);
		assert(0 == nRet);
#	endif
	}

public:
	void Notify_One() {
#	if defined WIN32
		if (NULL != m_event) { SetEvent(m_event); }
#	elif defined _LINUX
		int nRet = pthread_cond_signal(&m_event);
		assert(0 == nRet);
#	endif
	}

	void Notify_All() {
#	if defined WIN32
		// MSDN: Any number of waiting threads..., can be released while the object's state is signaled.
		if (NULL != m_event) { SetEvent(m_event); }
#	elif defined _LINUX
		int nRet = pthread_cond_broadcast(&m_event);
		assert(0 == nRet);
#	endif
	}

	void Wait(TMutex *mtx) {
#	if defined WIN32
		WaitForSingleObject(m_event, INFINITE);
#	elif defined _LINUX
		assert(NULL != mtx);
		if (NULL == mtx) {
			return;
		}
		TMutexImpl *pMtxImpl = (TMutexImpl *)mtx;
		pthread_mutex_t *p_mtx = (pthread_mutex_t *)pMtxImpl->GetData();
		pthread_cond_wait(&m_event, p_mtx);
#	endif
	}

	void Wait(unsigned long timeout/* millisecond */, TMutex *mtx) {
#	if defined WIN32
		WaitForSingleObject(m_event, timeout);
#	elif defined _LINUX
		assert(NULL != mtx);
		if (NULL == mtx) {
			return;
		}
		TMutexImpl *pMtxImpl = (TMutexImpl *)mtx;
		pthread_mutex_t *p_mtx = (pthread_mutex_t *)pMtxImpl->GetData();

		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec  += (timeout / 1000);
		ts.tv_nsec += (timeout % 1000) * 1000000;
		if (ts.tv_nsec >= 1000000000) { // 1s = 1000,000,000 ns
			ts.tv_nsec -= 1000000000;
			ts.tv_sec  += 1;
		}
		pthread_cond_timedwait(&m_event, p_mtx, &ts);
#	endif
	}

private:
#if defined WIN32
	HANDLE		   m_event;
#elif defined _LINUX
	pthread_cond_t m_event;
#endif
};

TEvent::TEvent() : m_pEvtImpl(new TEventImpl()) {
	assert(NULL != m_pEvtImpl);
}

TEvent::~TEvent() {
	delete m_pEvtImpl;
	m_pEvtImpl = 0;
}

void TEvent::Notify_One() {
	m_pEvtImpl->Notify_One();
}

void TEvent::Notify_All() {
	m_pEvtImpl->Notify_All();
}

void TEvent::Wait(TMutex *mtx) {
	m_pEvtImpl->Wait(mtx);
}

void TEvent::Wait(unsigned long timeout/* millisecond */, TMutex *mtx) {
	m_pEvtImpl->Wait(timeout, mtx);
}

class TSemaphoreImpl
{
public:
	 TSemaphoreImpl()
	 {
#	if defined WIN32
		 m_hSem = NULL;
#	elif defined _LINUX
		 memset(&m_semid, 0, sizeof(m_semid)); m_psid = 0;
		 memset(m_name, 0, sizeof(m_name));
#	endif
	 }

	~TSemaphoreImpl()
	{
#	if defined WIN32
		if (m_hSem != NULL) {
			CloseHandle(m_hSem); m_hSem = NULL;
		}
#	elif defined _LINUX
		if (0 == strlen(m_name)) {
			sem_destroy(&m_semid);
		} else {
			sem_close(m_psid);
			sem_unlink(m_name);
			memset(m_name, 0, sizeof(m_name));
		}
		memset(&m_semid, 0, sizeof(m_semid));
		m_psid = 0;
#	endif
	}

	bool Create(long nInitCount, long nMaxCount, const char *name = 0)
	{
		if (0 > nInitCount || 64 < nInitCount || 64 < nMaxCount || 0 >= nMaxCount) {
			return false;
		}

		if (nInitCount > nMaxCount) {
			nInitCount = nMaxCount;
		}

#	if defined WIN32
		if (NULL != m_hSem) {
			return true; // already created.
		}
		m_hSem = CreateSemaphoreA(NULL, nInitCount,  nMaxCount, name);
		if (NULL == m_hSem) {
			return false;
		}
#	elif defined _LINUX
		if (0 != m_psid) {
			return true; // already created.
		}
		if (NULL == name || 0 == strlen(name))
		{
			if (0 != sem_init(&m_semid, 0, nInitCount)) {
				return false;
			}
			m_psid = &m_semid;
		}
		else
		{
			if (256 < strlen(name)) {
			//	int * p = NULL;	*p = 3;
				assert(false);
				return false;
			}
			m_psid = sem_open(name, O_CREAT, 0644, nInitCount);
			if (SEM_FAILED == m_psid) {
				return false;
			}
			strcpy(m_name, name);
		}
#	endif
		return true;
	}

	bool Open(const char *name)
	{
		if (NULL == name || 0 == strlen(name)) {
			return false;
		}
#	if defined WIN32
		if (NULL != m_hSem) {
			return true; // already opened.
		}
		m_hSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name);
		if (NULL == m_hSem)	{
			return false;
		}
#	elif defined _LINUX
		if (0 != m_psid) {
			return true; // already opened.
		}
		if (256 < strlen(name)) {
			return false;
		}
		m_psid = sem_open(name, O_EXCL);
		if (SEM_FAILED == m_psid) {
			return false;
		}
		strcpy(m_name, name);
#	endif
		return true;
	}

public:
	bool Notify()
	{
#	if defined WIN32
		if (NULL == m_hSem)	{
			return false;
		}
		return ReleaseSemaphore(m_hSem, 1, NULL) ? true : false;
#	elif defined _LINUX
		if (0 == m_psid) {
			return false;
		}
		if (0 == strlen(m_name)) {
			return (0 == sem_post(&m_semid));
		} else {
			return (0 == sem_post(m_psid));
		}
#	endif
	}

	void Wait()
	{
#	if defined WIN32
		if (NULL == m_hSem)	{
			return;
		}
		WaitForSingleObject(m_hSem, INFINITE);
#	elif defined _LINUX
		if (0 == m_psid) {
			return;
		}
		if (0 == strlen(m_name)) {
			sem_wait(&m_semid);
		} else {
			sem_wait(m_psid);
		}
#	endif
	}

	void Wait(unsigned long timeout/* millisecond */)
	{
#	if defined WIN32
		if (NULL == m_hSem)	{
			return;
		}
		WaitForSingleObject(m_hSem, timeout);
#	elif defined _LINUX
		if (0 == m_psid) {
			return;
		}
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec  += (timeout / 1000);
		ts.tv_nsec += (timeout % 1000) * 1000000;
		if (ts.tv_nsec >= 1000000000) { // 1s = 1000,000,000 ns
			ts.tv_nsec -= 1000000000;
			ts.tv_sec  += 1;
		}
		if (0 == strlen(m_name)) {
			sem_timedwait(&m_semid, &ts);
		} else {
			sem_timedwait(m_psid,  &ts);
		}
#	endif
	}

private:
#if defined WIN32
	HANDLE m_hSem;
#elif defined _LINUX
	sem_t  m_semid;
	sem_t *m_psid;
	char   m_name[260];
#endif
};

TSemaphore::TSemaphore() : m_pSemImpl(new TSemaphoreImpl())
{
	assert(NULL != m_pSemImpl);
}

TSemaphore::~TSemaphore()
{
	delete m_pSemImpl;
	m_pSemImpl = NULL;
}


bool TSemaphore::Create(long nInitCount, long nMaxCount, const char *name/* = 0*/)
{
	return m_pSemImpl->Create(nInitCount, nMaxCount, name);
}

bool TSemaphore::Open(const char *name)
{
	return m_pSemImpl->Open(name);
}

bool TSemaphore::Notify()
{
	return m_pSemImpl->Notify();
}

void TSemaphore::Wait()
{
	m_pSemImpl->Wait();
}

void TSemaphore::Wait(unsigned long timeout/* millisecond */)
{
	m_pSemImpl->Wait(timeout);
}

/*
** implement of thread
*/
class ThreadImpl
{
private:
	ThreadImpl(const ThreadImpl&);
	ThreadImpl& operator=(const ThreadImpl&);

public:
	ThreadImpl();
	explicit ThreadImpl(Runnable *pRunnable);
	virtual ~ThreadImpl();

	bool SetRunnable(Runnable *pRunnable);
	Runnable* GetRunnable();

	unsigned long int GetThreadID() const;

	virtual void Run();

	bool Start();

	void Join();

	void Join(unsigned long timeout/* millisecond */);

	static void Sleep(long millis);

	static void yield();

	static unsigned long int GetCurrThreadID();

	void Terminate();

private:
	Runnable* m_target;

private:
#if defined WIN32
	unsigned long m_threadId;
	uintptr_t     m_hThread;
#elif defined _LINUX
	pthread_t     m_threadId;
#endif
};

#if defined WIN32
unsigned  __stdcall threadProc(void* param)
{
	ThreadImpl *p = (ThreadImpl*)param;
	if (NULL != p) {
		p->Run();
	}
	
	return 0;
}
#elif defined _LINUX
static void* ThreadProc(void* param)
{
	ThreadImpl *p = (ThreadImpl*)param;
	if (NULL != p) {
		p->Run();
	}

	return (void*)0;;
}
#endif

ThreadImpl::ThreadImpl() : m_target(0), m_threadId(0)
#if defined WIN32
, m_hThread(0)
#endif
{

}

ThreadImpl::ThreadImpl(Runnable *pRunnable) : m_target(pRunnable), m_threadId(0)
#if defined WIN32
, m_hThread(0)
#endif
{

}

ThreadImpl::~ThreadImpl()
{
	// 1.stop
	if (NULL != m_target) {
		m_target->Stop();
	}

	// 2.release thread
#if defined _WIN32_WCE	// special for wince
	if (0 != m_hThread) {
		WaitForSingleObject((HANDLE)m_hThread, 5000/*ms*/);
		Terminate();
	}
#else
#	if defined WIN32
	if (0 != m_hThread) {
		WaitForSingleObject((HANDLE)m_hThread, INFINITE);
		CloseHandle((HANDLE)m_hThread);
		m_hThread  = 0;
		m_threadId = 0;
	}
#	elif defined _LINUX
	if(0 != m_threadId) {
		pthread_cancel(m_threadId);
		pthread_join(m_threadId, NULL);
		m_threadId = 0;
	}
#	endif
#endif

	// 3.release runnable
	if (0 != m_target) {
		delete m_target;
		m_target = 0;
	}
}

bool ThreadImpl::SetRunnable(Runnable *pRunnable)
{
	if (NULL == pRunnable) {
		return false;
	}
	if (0 != m_threadId) {
		return false;
	}

	m_target = pRunnable;
	return true;
}

Runnable* ThreadImpl::GetRunnable()
{
	return m_target;
}

unsigned long int ThreadImpl::GetThreadID() const
{
	return m_threadId;
}

void ThreadImpl::Run()
{
	if (0 != m_target) {
		m_target->Run();
	}

#if defined WIN32
	::CloseHandle(reinterpret_cast<HANDLE>(m_hThread));
	m_hThread  = 0;
	m_threadId = 0;
#elif defined _LINUX
//	m_threadId = 0;		// Either pthread_join(3) or pthread_detach() should be called for each thread
	pthread_exit(NULL); // must call at last.
#endif
}

bool ThreadImpl::Start()
{
	if (0 != m_threadId) {
		return true; // start already
	}

#if defined _WIN32_WCE
	DWORD id;
	HANDLE hdlThread = ::CreateThread( // MSDN: If the function fails, the return value is NULL
		NULL, 
		0, 
		reinterpret_cast<LPTHREAD_START_ROUTINE>(threadProc),
		(void*)this,
		0, 
		&id
		);
	if (NULL == hdlThread) {
		return false;
	}
	m_hThread  = reinterpret_cast<uintptr_t>(hdlThread);
	m_threadId = (unsigned long)id;
#elif defined WIN32
	unsigned id;
	uintptr_t hdlThread = _beginthreadex( // MSDN: _beginthreadex returns 0 on an error
		NULL,
		0,
		threadProc,
		(void*)this,
		0,
		&id
		);
	if (0 == hdlThread) {
		return false;
	}
	m_hThread  = hdlThread;
	m_threadId = (unsigned long)id;
#elif defined _LINUX
	pthread_t tid = 0;
	int ret = pthread_create(&tid, NULL, ThreadProc, (void*)this);
	if (ret != 0) {
		return false;
	}
	m_threadId = tid;
#endif

	return true;
}

void ThreadImpl::Join()
{
#if defined WIN32
	if (0 != m_hThread) {
		WaitForSingleObject((HANDLE)m_hThread, INFINITE);
	}
#elif defined _LINUX
	if (0 != m_threadId) {
		pthread_join(m_threadId, NULL); m_threadId = 0;
	}
#endif
}

void ThreadImpl::Join(unsigned long timeout/* millisecond */)
{
#if defined WIN32
	if (0 != m_hThread) {
		WaitForSingleObject((HANDLE)m_hThread, timeout);
	}
#elif defined _LINUX
	if (0 != m_threadId) {
		void *status;
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec  += (timeout / 1000);
		ts.tv_nsec += (timeout % 1000) * 1000000;
		if (ts.tv_nsec >= 1000000000) { // 1s = 1000,000,000 ns
			ts.tv_nsec -= 1000000000;
			ts.tv_sec  += 1;
		}
		pthread_timedjoin_np(m_threadId, &status, &ts);
	}
#endif
}

/*static */void ThreadImpl::Sleep(long milliseconds)
{
#if defined WIN32
	::Sleep(milliseconds);
#elif defined _LINUX
	usleep(milliseconds * 1000); // usleep(/* microseconds */);
#endif
}

/*static */void ThreadImpl::yield()
{
#if defined WIN32
	::Sleep(1);
#elif defined _LINUX
	usleep(1*1000); // usleep(/* microseconds */);
#endif
}

/*static */unsigned long int ThreadImpl::GetCurrThreadID()
{
#if defined WIN32
	return (unsigned long int)GetCurrentThreadId();
#elif defined _LINUX
	return (unsigned long int)pthread_self();
#endif
}

void ThreadImpl::Terminate()
{
#if defined WIN32
	if (0 != m_hThread) {
		TerminateThread((HANDLE)m_hThread, 0);
		CloseHandle((HANDLE)m_hThread);
		m_hThread  = 0;
		m_threadId = 0;
	}
#elif defined _LINUX
	if (0 != m_threadId) {
		pthread_cancel(m_threadId);
		pthread_join(m_threadId, NULL);
		m_threadId = 0;
	}
#endif
}

Thread::Thread() : m_pTrdImpl(new ThreadImpl())
{
	assert(NULL != m_pTrdImpl);
}

Thread::Thread(Runnable *pRunnable) : m_pTrdImpl(new ThreadImpl(pRunnable))
{
	assert(NULL != m_pTrdImpl);
}

Thread::~Thread()
{
	delete m_pTrdImpl;
	m_pTrdImpl = 0;
}

bool Thread::SetRunnable(Runnable *pRunnable)
{
	return m_pTrdImpl->SetRunnable(pRunnable);
}

Runnable* Thread::GetRunnable()
{
	return m_pTrdImpl->GetRunnable();
}

unsigned long int Thread::GetThreadID() const
{
	return m_pTrdImpl->GetThreadID();
}

void Thread::Run()
{
	m_pTrdImpl->Run();
}

bool Thread::Start()
{
	return m_pTrdImpl->Start();
}

void Thread::Join()
{
	m_pTrdImpl->Join();
}

void Thread::Join(unsigned long timeout/* millisecond */)
{
	m_pTrdImpl->Join(timeout);
}

void Thread::Sleep(long millis)
{
	ThreadImpl::Sleep(millis);
}

void Thread::yield()
{
	ThreadImpl::yield();
}

unsigned long int Thread::GetCurrThreadID()
{
	return ThreadImpl::GetCurrThreadID();
}

void Thread::Terminate()
{
	m_pTrdImpl->Terminate();
}


} //~namespace threadm