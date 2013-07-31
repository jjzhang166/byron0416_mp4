#ifndef EVENT_H
#define EVENT_H


#include <sys/epoll.h>
#include <vector>
#include "Error.h"
#include "Thread.h"


using std::vector;

#define MAX_EVENTS 128

enum EVENT_TYPE
{
	EVENT_READ  = 0x01,
#define EVENTREAD EVENT_READ
	EVENT_WRITE = 0x02,
#define EVENTWRITE EVENT_WRITE
	EVENT_ERROR = 0x04,
#define EVENTERROR EVENT_ERROR
	EVENT_ET = 0x08
#define EVENTET EVENT_ET
};

class CEventEngin;

class CEvent: public CLogger
{
public:
	CEvent(CEventEngin*, CEvent*);
	virtual ~CEvent()      = 0;
	virtual int  GetFd()   = 0;
	virtual void OnRead()  = 0;
	virtual void OnWrite() = 0;
	virtual void OnError() = 0;
	virtual void OnSubEvent(CEvent*, ErrorCode) = 0;
protected:
	bool RegisterPriority();
	bool RegisterRD();
	bool RegisterWR();
	bool RegisterRW();
	bool Unregister();
	void ReturnSubEvent(ErrorCode err){m_Pre->OnSubEvent(this, err);}
private:
	bool Register(int tag);
protected:
	CEventEngin *m_Engin;
	CEvent *m_Pre;
	bool m_Added;
};

class CEventImplement: public CEvent
{
public:
	CEventImplement(CEventEngin*, CEvent*);
	void OnRead();
	void OnWrite();
	void OnError();
	void OnSubEvent(CEvent*, ErrorCode);
};

class CTimer: public CEventImplement
{
public:
	CTimer(CEventEngin *engin, CEvent *pre): CEventImplement(engin, pre), m_Fd(-1) {}
	~CTimer();
	/**
	 * @param val Nanosecond.
	 * @param inter Nanosecond.
	 */
	bool SetTimer(size_t val, size_t inter);
private:
	bool Initialize();
	void Uninitialize();
	// CEvent
	int  GetFd() {return m_Fd;}
	void OnRead();
private:
	int m_Fd;
};

class CEventEx: public CEvent
{
public:
	CEventEx(CEventEngin*, CEvent*);
	/**
	 * @param val Millisecond.
	 * @param inter Millisecond.
	 */
	virtual bool SetTimer(size_t val, size_t inter) = 0;
	virtual void OnTimer() = 0;
};

class CEventExImplement: public CEventEx
{
public:
	CEventExImplement(CEventEngin*, CEvent*);
	bool SetTimer(size_t val, size_t inter);
	void OnRead();
	void OnWrite();
	void OnError();
	void OnSubEvent(CEvent*, ErrorCode);
	void OnTimer();
private:
	CTimer m_Timer;
};

class CEventEngin: public CThread
{
public:
	CEventEngin();
	virtual ~CEventEngin();
	bool Initialize(ssize_t affinity = -1);
	bool AddPriority(CEvent *event);
	bool Add(CEvent *event, int tag);
	bool Mod(CEvent *event, int tag);
	bool Del(CEvent *event);
	bool Uninitialize();
private:
	void Thread();
private:
	int m_Fd;
	struct epoll_event m_Events[MAX_EVENTS];
	CEvent *m_Priority;
	vector<CEvent*> m_Delete;
	bool m_Doing;
};


#endif
