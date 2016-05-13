/*
** Show respect for D.R.Hipp <etilqs>
**
** 2016 February 28
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** Desc: a simple event template library.
** Mail: mean1hsu@gmail.com
** Vers: 0.0.1 <2016/03/05>
** Vers: 0.0.2 <2016/03/14>
** Vers: 0.0.3 <2016/03/31>
*/

#ifndef _EVENT_CENTER_H_
#define _EVENT_CENTER_H_

#include "EventLoop.h"
#include "Thread.h"
#include <assert.h>


namespace etl {


template <class Condition  = NullBinSem, class Atom = NullAtom,
		  class QueueLock  = NullLock,
		  class HandleLock = NullLock, 
		  template <class> class AutoLock = LockGuard, 
		  class Container  = std::deque<Event*> >
class EventCenter 
{
private:
	EventCenter(const EventCenter &r);
	EventCenter& operator = (const EventCenter &r);

public:
	typedef EventQueueImpl<QueueLock, AutoLock, Container>			MinEvtQueueImpl;
	typedef EventLoopImpl <Condition, Atom, HandleLock, AutoLock>	MinEvtLoopImpl;

	EventCenter() : m_pEvtQueue(NULL), m_pEvtLoop(NULL) {
		m_pEvtQueue = new MinEvtQueueImpl();	// create queue first
		m_pEvtLoop  = new MinEvtLoopImpl(m_pEvtQueue);
		assert(NULL != m_pEvtQueue);
		assert(NULL != m_pEvtLoop);
	}

	virtual ~EventCenter() {
		delete m_pEvtLoop;  m_pEvtLoop  = NULL;
		delete m_pEvtQueue; m_pEvtQueue = NULL;	// delete queue last
	}

	virtual EventLoop* GetEventLoop() {
		return m_pEvtLoop;
	}

private:
	MinEvtQueueImpl *m_pEvtQueue;
	MinEvtLoopImpl  *m_pEvtLoop;
};


template <class Runnable>
class EventRunnable : public Runnable
{
private:
	EventRunnable(const EventRunnable &r);
	EventRunnable& operator = (const EventRunnable &r);

public:
	EventRunnable(EventLoop *pEvtLoop) : m_pEvtLoop(pEvtLoop) {
		assert(NULL != m_pEvtLoop);
	}
	virtual ~EventRunnable() {
		m_pEvtLoop = NULL;
	}

	virtual void Run() {
		m_pEvtLoop->Loop();
	}
	virtual void Stop() {
		m_pEvtLoop->Stop();
	}

private:
	EventLoop *m_pEvtLoop;
};


} //~namespace etl


#endif // _EVENT_CENTER_H_