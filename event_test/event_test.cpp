// event_test.cpp : Defines the entry point for the console application.
//
#include "EventCenter.h"

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


class MyEvent1 : public etl::Event
{
public:
	MyEvent1(const char *data, unsigned int id) : Event(id) {
		memset(m_szData, 0, sizeof(m_szData));
		if (0 != data) {
			strcpy_ss(m_szData, 256, data);
		}
	}
	virtual ~MyEvent1() {
	}
	virtual MyEvent1* Clone() {
		return new MyEvent1(*this);
	}
	virtual void* GetData()  const { 
		return (void*)(char*)&m_szData[0];
	}
private:
	char m_szData[256];
};

class MyEvent2 : public etl::Event
{
public:
	virtual ~MyEvent2() {
		delete []m_pData;
	}
	MyEvent2(const char *data, unsigned int id) : Event(id), m_pData(0) {
		if (0 != data && 0 < strlen(data)) {
			const int LEN = strlen(data) + 1;
			m_pData = new char[LEN];
			strcpy_ss(m_pData, LEN, data);
		}
	}
	MyEvent2(const MyEvent2 &r) : Event(r.m_id) {
		if (0 != r.m_pData && 0 < strlen(r.m_pData)) {
			const int LEN = strlen(r.m_pData) + 1;
			m_pData = new char[LEN];
			strcpy_ss(m_pData, LEN, r.m_pData);
		}
	}
	MyEvent2& operator = (const MyEvent2 &r) {
		if (this != &r) {
			m_id = r.m_id;
			delete []m_pData;
			m_pData = 0;
			if (r.m_pData && 0 < strlen(r.m_pData)) {
				const int LEN = strlen(r.m_pData) + 1;
				m_pData = new char[LEN];
				strcpy_ss(m_pData, LEN, r.m_pData);
			}
		}
		return *this;
	}
	virtual MyEvent2* Clone() {
		return new MyEvent2(*this);
	}
	virtual void* GetData()  const { 
		return (void*)m_pData;
	}
private:
	char *m_pData;
};

typedef etl::EventCenter<threadm::TSemaphore, threadm::TAtom, threadm::TMutex, threadm::TMutex> EventCenterDef;
typedef etl::EventRunnable<threadm::Runnable> EventRunnableDef;

class MyEventCenter : public EventCenterDef
{
public:
	enum {
		EVENT_DOOR_OPENED = 0,
		EVENT_DOOR_CLOSED = 1,
		EVENT_MAIN_EXITED = 2,
	};

public:
	MyEventCenter() : EventCenter(), m_hdlOpened(0), m_hdlClosed(0) {
		m_hdlOpened = etl::NewHandler<MyEventCenter,void>(this, &MyEventCenter::func_on_door_opened);
		m_hdlClosed = etl::NewHandler<MyEventCenter,void>(this, &MyEventCenter::func_on_door_closed);
		m_hdlExited = etl::NewHandler<MyEventCenter,void>(this, &MyEventCenter::func_on_main_exited);

		GetEventLoop()->RegisterEventHandler(EVENT_DOOR_OPENED, m_hdlOpened);
		GetEventLoop()->RegisterEventHandler(EVENT_DOOR_CLOSED, m_hdlClosed);
		GetEventLoop()->RegisterEventHandler(EVENT_MAIN_EXITED, m_hdlExited);
	}
	~MyEventCenter() {
		GetEventLoop()->UnRegisterEventHandler(EVENT_DOOR_OPENED, m_hdlOpened);
		GetEventLoop()->UnRegisterEventHandler(EVENT_DOOR_CLOSED, m_hdlClosed);
		GetEventLoop()->UnRegisterEventHandler(EVENT_MAIN_EXITED, m_hdlExited);

		etl::DelHandler(m_hdlOpened);
		etl::DelHandler(m_hdlClosed);
		etl::DelHandler(m_hdlExited);
		m_hdlOpened = 0;
		m_hdlClosed = 0;
		m_hdlExited = 0;
	}

	void func_on_door_opened(etl::Event* evt) {
		cout << "func_on_door_opened: ";
		if (0 != evt) {
			char * pData = (char*)evt->GetData();
			cout << pData;
		}
		cout << endl;
	}

	void func_on_door_closed(etl::Event* evt) {
		cout << "func_on_door_closed: ";
		if (0 != evt) {
			char * pData = (char*)evt->GetData();
			cout << pData;
		}
		cout << endl;
	}

	void func_on_main_exited(etl::Event* evt) {
		cout << "func_on_main_exited: ";
		if (0 != evt) {
			char * pData = (char*)evt->GetData();
			cout << pData;
		}
		cout << endl;

		GetEventLoop()->Stop();
	}

private:
	etl::EventHandler *m_hdlOpened;
	etl::EventHandler *m_hdlClosed;
	etl::EventHandler *m_hdlExited;
};

class PostEventRunnable : public threadm::Runnable
{
public:
	PostEventRunnable(etl::EventLoop *pEvtLoop) : m_pEvtLoop(pEvtLoop) {
	}
	virtual ~PostEventRunnable() {
		m_pEvtLoop = 0;
	}

	virtual void Run() {
		do
		{
			threadm::Thread::Sleep(1000);
			MyEvent1 evt1("pull  the man", MyEventCenter::EVENT_DOOR_OPENED);
			m_pEvtLoop->PostEvent(&evt1);

			threadm::Thread::Sleep(2000);
			MyEvent2 evt2("untie the dog", MyEventCenter::EVENT_DOOR_CLOSED);
			m_pEvtLoop->PostEvent(&evt2);

			threadm::Thread::Sleep(3000);
			MyEvent1 evt3("the world all clear...", MyEventCenter::EVENT_MAIN_EXITED);
			m_pEvtLoop->PostEvent(&evt3);

		} while(0);
	}

private:
	etl::EventLoop *m_pEvtLoop;
};

int _tmain(int argc, _TCHAR* argv[])
{
#if 0

	MyEventCenter cen;
	threadm::Thread trd(new EventRunnableDef(cen.GetEventLoop()));

	trd.Start();

	trd.Join(1000);
	MyEvent1 evt1("pull  the man", MyEventCenter::EVENT_DOOR_OPENED);
	cen.GetEventLoop()->PostEvent(&evt1);
	
	trd.Join(2000);
	MyEvent2 evt2("untie the dog", MyEventCenter::EVENT_DOOR_CLOSED);
	cen.GetEventLoop()->PostEvent(&evt2);

	trd.Join(3000);
	trd.GetRunnable()->Stop();
	trd.Join();

#else

	MyEventCenter cen;
	threadm::Thread trds[10];
	for (int i = 0; i < sizeof(trds)/sizeof(trds[0]); ++i) {
		trds[i].SetRunnable(new PostEventRunnable(cen.GetEventLoop()));
		trds[i].Start();
	}

	cen.GetEventLoop()->Loop();

	for (int i = 0; i < sizeof(trds)/sizeof(trds[0]); ++i) {
		trds[i].Join();
	}

#endif

	return 0;
}