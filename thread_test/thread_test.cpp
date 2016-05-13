// thread_test.cpp : Defines the entry point for the console application.
//
#include "Thread.h"
#include "ThreadPool.h"

#include <string.h>	// strcpy
#include <iostream>
using namespace std;

#if defined WIN32
#include <tchar.h>
#elif defined _LINUX
#define _tmain main
typedef char _TCHAR;
#endif

#ifndef strcpy_ss
#if defined WIN32
# define strcpy_ss(dst, len, src)			strcpy_s(dst, len, src)
#elif defined _LINUX
# define strcpy_ss(dst, len, src)			strcpy  (dst, src)
#endif
#endif


class Print100 : public threadm::Runnable
{
public:
	Print100(const char *szText, int loop) : m_nLoop(loop), m_bExit(0) {
		memset(m_szText, 0, sizeof(m_szText));
		if (szText) {
			strcpy_ss(m_szText, 256, szText);
		}
	}
	virtual ~Print100() {

	}
	virtual void Run() {
		for (int i = 0; i < m_nLoop && !m_bExit.Read(); ++i) {
			cout << i << "\t: " << m_szText << endl;
		}
	}
	virtual void Stop() {
		m_bExit.Set(1);
	}

private:
	int			   m_nLoop;
	char		   m_szText[256];
	threadm::TAtom m_bExit;
};


int _tmain(int argc, _TCHAR* argv[])
{
	// 1.a simple thread
	threadm::Thread trd1(new Print100("Hello World!", 100));
	trd1.Start();
	cout << "thread " << trd1.GetThreadID() << " is running" << endl;
	trd1.Join();

	// 2.stop a thread task(runnable)
	threadm::Thread trd2(new Print100("Stop Stop Stop!", 5000));
	trd2.Start();
	cout << "thread " << trd2.GetThreadID() << " is running" << endl;
	trd2.Join(10);
	trd2.GetRunnable()->Stop();
	trd2.Join();

	// 3.threadpool test
	threadm::ThreadPool pool;
	if (!pool.Create(5, 10, 8)) {
		return 0;
	}

	for (int i = 0; i < 10; ++i) {
		pool.Execute(new Print100("Pool Pool Pool!", 300));
	}

	pool.Destroy();

	return 0;
}