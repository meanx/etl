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
** Vers: 0.0.2 <2016/04/01>
*/

#ifndef _EVENT_H_
#define _EVENT_H_

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


namespace etl {


/*
** event entity
*/
class Event
{
public:
	explicit Event(unsigned short nID) : m_id(nID) { } // x86, nID <= 65535

	virtual ~Event() {}

	virtual Event* Clone() { return new Event(*this); }

	virtual unsigned short GetID() const { return m_id; }
	
	virtual void* GetData() const { return NULL; }

protected:
	unsigned short m_id;
};


} //~namespace etl


#endif //~_EVENT_H_