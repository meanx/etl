/*
** Show respect for D.R.Hipp <etilqs>
**
** 2016 February 17
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** Mail: mean1hsu@gmail.com
** Vers: 0.0.1 <2016/02/22>
** Vers: 0.0.2 <2016/02/28>
** Vers: 0.0.3 <2016/03/31> fixed bugs on Linux.
*/

#ifndef _THREAD_H_
#define _THREAD_H_

#include "ThreadLink.h"


namespace threadm {


/*
** atomic operator
*/
THREAD_API long AtomicSet(volatile long *pAtom, long i);
THREAD_API long AtomicAdd(volatile long *pAtom, long i);
THREAD_API long AtomicSub(volatile long *pAtom, long i);
THREAD_API long AtomicInc(volatile long *pAtom);
THREAD_API long AtomicDec(volatile long *pAtom);

class THREAD_CLASS_DECL TAtom
{
public:
	THREAD_CLASS_API explicit TAtom(long val = 0) : _val(val) {}
	THREAD_CLASS_API inline long Read() { return _val; }  
	THREAD_CLASS_API inline void Set(long i) { AtomicSet(&_val, i); } 
	THREAD_CLASS_API inline void Add(int i)  { AtomicAdd(&_val, i); } 
	THREAD_CLASS_API inline void Sub(int i)  { AtomicSub(&_val, i); } 
	THREAD_CLASS_API inline void Inc() { AtomicInc(&_val); }  
	THREAD_CLASS_API inline void Dec() { AtomicDec(&_val); } 
 
private:
	volatile long _val;
};

/*
** thread lock
*/
class THREAD_CLASS_DECL TLock
{
public:
	THREAD_CLASS_API virtual ~TLock() {};

public:
	THREAD_CLASS_API virtual void Lock()   = 0;
	THREAD_CLASS_API virtual void Unlock() = 0;
};

class THREAD_CLASS_DECL TAutoLock
{
public:
	THREAD_CLASS_API  TAutoLock(TLock &lk) : m_lk(lk)
	{
		m_lk.Lock();
	}
	THREAD_CLASS_API ~TAutoLock()
	{
		m_lk.Unlock();
	}

private:
	TLock &m_lk;
};

class THREAD_CLASS_DECL TReentrantLock : public TLock {
};

class TMutexImpl;
class THREAD_CLASS_DECL TMutex : public TReentrantLock
{
private:
	TMutex(const TMutex&);
	TMutex& operator=(const TMutex&);

public:
	THREAD_CLASS_API  TMutex();

	THREAD_CLASS_API ~TMutex();

	THREAD_CLASS_API virtual void Lock();

	THREAD_CLASS_API virtual bool TryLock();

	THREAD_CLASS_API virtual void Unlock();

private:
	TMutexImpl* m_pMutexImp;
};

/*
** condition variable : Event (a binary semaphore)
*/
class TEventImpl;
class THREAD_CLASS_DECL TEvent
{
public:
	THREAD_CLASS_API  TEvent();
	THREAD_CLASS_API ~TEvent();

public:
	THREAD_CLASS_API
	void Notify_One();

	THREAD_CLASS_API
	void Notify_All();

	THREAD_CLASS_API
	void Wait(TMutex *mtx);

	// If timeout is zero, the function tests the object's state and returns immediately.
	THREAD_CLASS_API
	void Wait(unsigned long timeout/* millisecond */, TMutex *mtx);

private:
	TEventImpl *m_pEvtImpl;
};

/*
** condition variable : Semaphore (a multiple semaphore)
*/
class TSemaphoreImpl;
class THREAD_CLASS_DECL TSemaphore
{
public:
	THREAD_CLASS_API  TSemaphore();
	THREAD_CLASS_API ~TSemaphore();

	// 0 <= nInitCount && nInitCount <= 64
	// 0 <  nMaxCount  && nMaxCount  <= 64
	THREAD_CLASS_API bool Create(long nInitCount, long nMaxCount, const char *name = 0);
	THREAD_CLASS_API bool Open(const char *name);

public:
	THREAD_CLASS_API
	bool Notify();

	THREAD_CLASS_API
	void Wait();

	// If timeout is zero, the function tests the object's state and returns immediately.
	THREAD_CLASS_API
	void Wait(unsigned long timeout/* millisecond */);

private:
	TSemaphoreImpl *m_pSemImpl;
};

/*
** thread task interface : Runnable
*/
class THREAD_CLASS_DECL Runnable
{
public:
	THREAD_CLASS_API virtual ~Runnable()   {};
	THREAD_CLASS_API virtual void Run()   = 0;
	THREAD_CLASS_API virtual void Stop()   {};
	THREAD_CLASS_API virtual void Pause()  {};
	THREAD_CLASS_API virtual void Resume() {};
};

/*
** ah~, ah~, a simple thread
*/
class ThreadImpl;
class THREAD_CLASS_DECL Thread
{
private:
	Thread(const Thread&);
	Thread& operator=(const Thread&);

public:
	THREAD_CLASS_API Thread();
	THREAD_CLASS_API explicit Thread(Runnable *pRunnable);
	THREAD_CLASS_API virtual ~Thread();

	/**
	 *@func : SetRunnable
	 *@desc : must call this func before Start().
	 */
	THREAD_CLASS_API bool SetRunnable(Runnable *pRunnable);
	THREAD_CLASS_API Runnable* GetRunnable();

	THREAD_CLASS_API unsigned long int GetThreadID() const;

	THREAD_CLASS_API virtual void Run();

	THREAD_CLASS_API bool Start();

	THREAD_CLASS_API void Join();

	THREAD_CLASS_API void Join(unsigned long timeout/* millisecond */);

	THREAD_CLASS_API
	static void Sleep(long millis);

	THREAD_CLASS_API
	static void yield();

	THREAD_CLASS_API
	static unsigned long int GetCurrThreadID();

	//@ deprecated
	THREAD_CLASS_API
	void Terminate();

private:
	ThreadImpl *m_pTrdImpl;
};


} //~namespace threadm


#endif // __THREAD_H_