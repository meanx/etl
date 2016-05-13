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
*/

#ifndef _EVENTHANDLER_H_
#define _EVENTHANDLER_H_

#include "Event.h"


namespace etl {


class EventHandler
{
public:
	virtual ~EventHandler() {};
	
	virtual void HandleEvent(Event* pEvt) = 0;
};
	

inline void DelHandler(EventHandler *&pHandler)
{
	delete pHandler;
	pHandler = NULL;
}

template <typename R>
class FuncEventHandler : public EventHandler
{
	typedef R (*PF)(Event* );
public:
	FuncEventHandler(PF pf) : m_pf(pf) {

	}
	virtual ~FuncEventHandler() {
		m_pf = NULL; 
	}

	virtual void HandleEvent(Event* pEvt) {
		if (m_pf) {
			m_pf(pEvt);
		}
	}

private:
	PF m_pf;
};

template <typename R>
inline EventHandler* NewHandler(R (*function)(Event*))
{
	return new FuncEventHandler<R>(function);
}

template <typename T, typename R>
class MemFuncEventHandler : public EventHandler
{
	typedef R (T::*PMF)(Event* );
public:
	MemFuncEventHandler(T *pObj, PMF memfunc) : m_pmf(memfunc), m_t(pObj) {

	}
	virtual ~MemFuncEventHandler() {
		m_pmf = NULL; 
		m_t  = NULL;
	}

	virtual void HandleEvent(Event* pEvt) {
		if (m_t && m_pmf) {
			(m_t->*m_pmf)(pEvt);
		}
	}

private:
	PMF m_pmf;
	T * m_t;
};

template <typename T, typename R>
inline EventHandler* NewHandler(T* pObj, R (T::*memfunc)(Event*))
{
	return new MemFuncEventHandler<T, R>(pObj, memfunc);
}

template <typename T, typename R>
inline EventHandler* NewHandler(const T* pObj, R (T::*memfunc)(Event*))
{
	return new MemFuncEventHandler<T, R>(pObj, memfunc);
}
#if 0
template <typename T, typename R>
inline EventHandler* NewHandler(T* const pObj, R (T::*memfunc)(Event*))
{
	return new MemFuncEventHandler<T, R>(pObj, memfunc);
}
#endif
template <typename T, typename R>
inline EventHandler* NewHandler(T& obj, R (T::*memfunc)(Event*))
{
	return new MemFuncEventHandler<T, R>(&obj, memfunc);
}


} //~namespace etl


#endif //~_EVENTHANDLER_H_