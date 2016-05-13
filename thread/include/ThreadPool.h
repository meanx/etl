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
** Vers: 0.0.1 <2016/02/28>
*/

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "Thread.h"


namespace threadm {


class ThreadPoolImpl;
class THREAD_CLASS_DECL ThreadPool {
public:
	THREAD_CLASS_API  ThreadPool();
	THREAD_CLASS_API ~ThreadPool();

	THREAD_CLASS_API bool Create(unsigned int nMinThread, unsigned int nMaxThread, unsigned int nMaxPendingTask);
	
	THREAD_CLASS_API void Destroy();

	THREAD_CLASS_API bool Execute(Runnable *pRunnable);

	THREAD_CLASS_API void Statistic(unsigned int &nThread, unsigned int &nTask);

private:
	ThreadPoolImpl *m_pPoolImpl;
};


} //~namespace threadm


#endif // _THREADPOOL_H_