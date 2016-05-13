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
** Vers: 0.0.2 <2016/03/31>
*/

#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_

#include "Event.h"
#include "EventHandler.h"
#include "EventQueue.h"

#include <assert.h>
#include <map>


namespace etl {


/*
** EventLoop definition
*/
class EventLoop
{
public:
	virtual ~EventLoop() {};

	virtual void SendEvent(Event* pEvt) = 0;
	virtual void PostEvent(Event* pEvt) = 0;

	virtual bool RegisterEventHandler(unsigned short evtId, EventHandler *hdl) = 0;
	virtual EventHandler* UnRegisterEventHandler(unsigned short evtId, EventHandler *hdl) = 0;

	virtual void Loop() = 0;
	virtual void Stop() = 0;
};


class NullBinSem { // binary semaphore
public:
	inline bool Create(long nInitCount, long nMaxCount, const char *name = 0) { return true; };
	inline bool Notify() { return true; };
	inline void Wait() {};
	inline void Wait(unsigned long timeout/* millisecond */) {};
};

class Non_Atom {
public:
	explicit Non_Atom(long val = 0) : _val(val) {}
	inline long Read() { return _val; }
	inline void Set(long i) { _val = i; }
private:
	volatile long _val;
};

template <class Condition = NullBinSem, class Atom = Non_Atom, class Lock = NullLock, template <class> class AutoLock = LockGuard>
class EventLoopImpl : public EventLoop
{
	typedef AutoLock<Lock>	AutoLockType;
public:
	EventLoopImpl(EventQueue *que);
	virtual ~EventLoopImpl();

	virtual void SendEvent(Event* pEvt);
	virtual void PostEvent(Event* pEvt);

	virtual bool RegisterEventHandler(unsigned short evtId, EventHandler *hdl);
	virtual EventHandler* UnRegisterEventHandler(unsigned short evtId, EventHandler *hdl);

	virtual void Loop();
	virtual void Stop();

private:
	void HandleEvent(Event* pEvt);
	void HandlePostEvent();

private:
	EventQueue *m_pQue;
	Lock        m_lkhd;
	Condition   m_cond;
	Atom        m_bQuit;
	typedef std::multimap<unsigned short, EventHandler*> EventHandlers;
	typedef std::multimap<unsigned short, EventHandler*>::iterator EHdlerIterator;
	EventHandlers m_eventHandlers;
};

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
EventLoopImpl<Condition, Atom, Lock, AutoLock>::EventLoopImpl(EventQueue *que) : m_pQue(que), m_bQuit(0)
{
	m_cond.Create(0, 1, "EventLoopImpl");
	assert(NULL != m_pQue);
}
template <class Condition, class Atom, class Lock, template <class> class AutoLock>
EventLoopImpl<Condition, Atom, Lock, AutoLock>::~EventLoopImpl()
{
	Stop();
	m_pQue = NULL;
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
void EventLoopImpl<Condition, Atom, Lock, AutoLock>::SendEvent(Event* pEvt)
{
	if (NULL == pEvt) {
		return;
	}
	HandleEvent(pEvt);
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
void EventLoopImpl<Condition, Atom, Lock, AutoLock>::PostEvent(Event* pEvt)
{
	if (NULL == pEvt) {
		return;
	}
	if (m_pQue->AddEvent(pEvt)) {
		m_cond.Notify();
	}
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
bool EventLoopImpl<Condition, Atom, Lock, AutoLock>::RegisterEventHandler(unsigned short evtId, EventHandler *hdl)
{
	if (NULL == hdl) {
		return false;
	}
	unsigned short id = evtId;
	std::pair<unsigned short, EventHandler*> pair = std::make_pair(id, hdl);

	AutoLockType atlk(m_lkhd);
	m_eventHandlers.insert(pair);

	return true;
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
EventHandler* EventLoopImpl<Condition, Atom, Lock, AutoLock>::UnRegisterEventHandler(unsigned short evtId, EventHandler *hdl)
{
	if (NULL == hdl) {
		return NULL;
	}

	unsigned short id = evtId;
	EventHandler* retHdler = NULL;

	AutoLockType atlk(m_lkhd);

	std::size_t cnt = m_eventHandlers.count(id);
	EHdlerIterator iter = m_eventHandlers.find(id);
	EHdlerIterator itrDel = m_eventHandlers.end();
	for (std::size_t i = cnt; i > 0; i--)
	{
		if (iter != m_eventHandlers.end()) {
			std::pair<unsigned short, EventHandler*> pair = *iter;
			if (pair.second == hdl) {
				itrDel = iter;
				break;
			}
			++iter;
		}
	}

	if (itrDel != m_eventHandlers.end()) {
		std::pair<unsigned short, EventHandler*> pair = *itrDel;
		retHdler = pair.second;
		m_eventHandlers.erase(itrDel);
	}

	return retHdler;
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
void EventLoopImpl<Condition, Atom, Lock, AutoLock>::Loop()
{
	HandlePostEvent();
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
void EventLoopImpl<Condition, Atom, Lock, AutoLock>::Stop()
{
	m_bQuit.Set(1);
	m_cond.Notify();
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
void EventLoopImpl<Condition, Atom, Lock, AutoLock>::HandleEvent(Event* pEvt)
{
	if (NULL == pEvt) {
		return;
	}

	unsigned short typeId = pEvt->GetID();
	EHdlerIterator iter = m_eventHandlers.find(typeId);
	std::size_t cnt = m_eventHandlers.count(typeId);
	for (std::size_t i = cnt; i > 0; i--)
	{
		if (iter != m_eventHandlers.end()) {
			std::pair<unsigned short, EventHandler*> pair = *iter;
			EventHandler *handler = pair.second;
			handler->HandleEvent(pEvt);
			++iter;
		}
	}
}

template <class Condition, class Atom, class Lock, template <class> class AutoLock>
void EventLoopImpl<Condition, Atom, Lock, AutoLock>::HandlePostEvent()
{
	do
	{
		Event *pEvt = m_pQue->GetEvent();
		if (NULL == pEvt) {
			m_cond.Wait(100);
			if (m_bQuit.Read()) {
				break;
			}
			continue;
		}

		HandleEvent(pEvt);

		delete pEvt;
		pEvt = NULL;

		if (m_bQuit.Read()) {
			break;
		}

	} while (1);
}


} //~namespace etl


#endif //~_EVENT_LOOP_H_