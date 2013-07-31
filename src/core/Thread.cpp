#include <errno.h>
#include <string.h>
#include "Thread.h"


/** CThread */

CThread::CThread(ssize_t affinity): m_Thread(0), m_Affinity(affinity), m_Run(true)
{
}

CThread::~CThread()
{
	Stop();
}

bool CThread::Run()
{
	if(0 != pthread_create(&m_Thread, NULL, __Thread, this))
	{
		LOG_FATAL("Create new thread failed(" << strerror(errno) << ").");

		return false;
	}
	else
		return true;
}

bool CThread::SetAffinity(ssize_t affinity)
{
	if(affinity >= 0)
	{
		m_Affinity = affinity;
		if(m_Thread > 0)
		{
			cpu_set_t mask;
			CPU_ZERO(&mask);
			CPU_SET(m_Affinity, &mask);
			if(pthread_setaffinity_np(m_Thread, sizeof(mask), &mask) < 0)
			{
				LOG_ERROR("Set thread affinity failed for cpu #" << m_Affinity << ".");

				return false;
			}
			else
				return true;
		}
		else
			return false;
	}
	else
		return false; 
}

bool CThread::Stop()
{
	if(m_Thread > 0)
	{
		m_Run = false;
		pthread_join(m_Thread, NULL);

		LOG_INFO("Thread " << m_Thread << " is terminated.");

		m_Thread = 0;
		m_Run = true;
		m_Affinity = -1;
	}

	return true;
}

void* CThread::__Thread(void *arg)
{
	CThread *thread = static_cast<CThread*>(arg);

	thread->SetAffinity(thread->m_Affinity);

	while(thread->m_Run)
		thread->Thread();

	return arg;
}
