#ifndef THREAD_H
#define THREAD_H


#include <pthread.h>
#include "Log.h"


class CMutex
{
public:
	CMutex()       {pthread_mutex_init(&m_Mutex, NULL);}
	~CMutex()      {pthread_mutex_destroy(&m_Mutex);}
	pthread_mutex_t* GetHandle() {return &m_Mutex;}
	bool TryLock() {return 0 == pthread_mutex_trylock(&m_Mutex) ? true : false;}
	void Lock()    {pthread_mutex_lock(&m_Mutex);}
	void Unlock()  {pthread_mutex_unlock(&m_Mutex);}
private:
	pthread_mutex_t m_Mutex;
};

class CAutoMutex
{
public:
	CAutoMutex(CMutex *mutex): m_Mutex(mutex) {m_Mutex->Lock();}
	~CAutoMutex() {m_Mutex->Unlock();}
private:
	CMutex *m_Mutex;
};

class CRWLock
{
public:
	CRWLock()    {pthread_rwlock_init(&m_Lock, NULL);   }
	~CRWLock()   {pthread_rwlock_destroy(&m_Lock);      }
	int RDLock() {return pthread_rwlock_rdlock(&m_Lock);}
	int WRLock() {return pthread_rwlock_wrlock(&m_Lock);}
	int UNLock() {return pthread_rwlock_unlock(&m_Lock);}
private:
	pthread_rwlock_t m_Lock;
};

class CThread: public CLogger
{
public:
	CThread(ssize_t affinity=-1);
	virtual ~CThread();
	bool Run();
	bool SetAffinity(ssize_t affinity);
	bool Stop();
private:
	virtual void Thread() = 0;
	static void* __Thread(void*);
private:
	pthread_t m_Thread;
	ssize_t m_Affinity;
	volatile bool m_Run;
};


#endif
