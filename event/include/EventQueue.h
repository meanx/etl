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
** Vers: 0.0.1 <2016/03/02>
** Vers: 0.0.2 <2016/03/31>
*/

#ifndef _EVENTQUEUE_H_
#define _EVENTQUEUE_H_

#include "Event.h"
#include <deque>


namespace etl {


class EventQueue
{
public:
	virtual ~EventQueue() {};

	virtual bool   AddEvent(Event*) = 0;
	virtual Event* GetEvent() = 0;
	virtual Event* PeekEvent() = 0;
};

class NullLock {
public:
	inline void Lock()   {};
	inline void Unlock() {};
};

template <class Lock>
class LockGuard {
public:
	 LockGuard(Lock &lk) : m_lk(lk) { m_lk.Lock(); }
	~LockGuard() { m_lk.Unlock(); }
private:
	Lock &m_lk;
};

template <class Lock = NullLock, template <class> class AutoLock = LockGuard, class Container = std::deque<Event*> >
class EventQueueImpl : public EventQueue
{
	typedef AutoLock<Lock>	AutoLockType;
public:
	EventQueueImpl();
	virtual ~EventQueueImpl();

	virtual bool   AddEvent(Event*);
	virtual Event* GetEvent();
	virtual Event* PeekEvent();

private:
	Lock      m_lkEvts;
	Container m_events;
};

template <class Lock, template <class> class AutoLock, class Container>
EventQueueImpl<Lock, AutoLock, Container>::EventQueueImpl()
{

}

template <class Lock, template <class> class AutoLock, class Container>
EventQueueImpl<Lock, AutoLock, Container>::~EventQueueImpl()
{
	AutoLockType atlk(m_lkEvts);
	while (!m_events.empty()) {
		Event *pEvt = (Event*)m_events.front();
		m_events.pop_front();
		delete pEvt;
		pEvt = NULL;
	}
}

template <class Lock, template <class> class AutoLock, class Container>
bool EventQueueImpl<Lock, AutoLock, Container>::AddEvent(Event* pEvt) 
{
	if (NULL == pEvt) {
		return false;
	}

	AutoLockType atlk(m_lkEvts);
	m_events.push_back(pEvt->Clone());

	return true;
}

template <class Lock, template <class> class AutoLock, class Container>
Event* EventQueueImpl<Lock, AutoLock, Container>::GetEvent()
{
	Event* pEvt = NULL;

	AutoLockType atlk(m_lkEvts);
	if (m_events.empty()) {
		return NULL;
	}
	pEvt = (Event*)m_events.front();
	m_events.pop_front();

	return pEvt;
}

template <class Lock, template <class> class AutoLock, class Container>
Event* EventQueueImpl<Lock, AutoLock, Container>::PeekEvent()
{
	if (m_events.empty()) {
		return NULL;
	}
	return (Event*)m_events.front();
}


} //~namespace etl


#endif // _EVENTQUEUE_H_