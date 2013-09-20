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
	ET_READ = 0x01,
#define ETREAD ET_READ
	ET_WRITE = 0x02,
#define ETWRITE ET_WRITE
	ET_ERROR = 0x04,
#define ETERROR ET_ERROR
	ET_ET = 0x08
#define ETET ET_ET
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
	virtual void OnSubEvent(const CEvent*, ErrorCode) = 0;
protected:
	bool RegisterPriority();
	bool RegisterRD();
	bool RegisterWR();
	bool RegisterRW();
	bool Unregister();
	void ReturnEvent(ErrorCode err) {m_Pre->OnSubEvent(this, err);}
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
	void OnSubEvent(const CEvent*, ErrorCode);
};

class CTimer: public CEventImplement
{
public:
	CTimer(CEventEngin *engin, CEvent *pre): CEventImplement(engin, pre), m_Fd(-1) {}
	~CTimer();
	/**
	 * @param value Nanosecond.
	 * @param interval Nanosecond.
	 */
	bool SetTimer(size_t value, size_t interval);
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
	 * @param value Millisecond.
	 * @param interval Millisecond.
	 */
	virtual bool SetTimer(size_t value, size_t interval) = 0;
	virtual void OnTimer() = 0;
};

class CEventExImplement: public CEventEx
{
public:
	CEventExImplement(CEventEngin*, CEvent*);
	bool SetTimer(size_t value, size_t interval);
	void OnRead();
	void OnWrite();
	void OnError();
	void OnSubEvent(const CEvent*, ErrorCode);
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
	bool AddPriority(CEvent*);
	bool Add(CEvent*, int tag);
	bool Mod(CEvent*, int tag);
	bool Del(CEvent*);
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
