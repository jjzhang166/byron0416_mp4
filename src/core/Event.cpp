#include <sys/timerfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "Event.h"


/** CEvent */

CEvent::CEvent(CEventEngin *engin, CEvent *pre):
	m_Engin(engin), m_Pre(pre), m_Added(false)
{
}

CEvent::~CEvent()
{
	assert(m_Added == false);
}

bool CEvent::RegisterPriority()
{
	if(m_Engin)
	{
		if(true == m_Added)
		{
			if(false == Unregister())
				return false;
			else
				m_Added = false;
		}

		if(true == m_Engin->AddPriority(this))
		{
			m_Added = true;

			return true;
		}
		else
			return false;
	}
	else
		return false;
}

bool CEvent::RegisterRD()
{
	return Register(ETREAD);
}

bool CEvent::RegisterWR()
{
	return Register(ETWRITE);
}

bool CEvent::RegisterRW()
{
	return Register(ETREAD|ETWRITE);
}

bool CEvent::Unregister()
{
	if(m_Added)
	{
		if(m_Engin)
		{
			if(true == m_Engin->Del(this))
			{
				m_Added = false;

				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return true;
}

inline bool CEvent::Register(int tag)
{
	if(m_Engin)
	{
		if(false == m_Added)
		{
			if(true == m_Engin->Add(this, tag|ETET))
			{
				m_Added = true;

				return true;
			}
			else
				return false;
		}
		else
			return m_Engin->Mod(this, tag|ETET);
	}
	else
		return false;
}


/** CEventImplement */

CEventImplement::CEventImplement(CEventEngin *engin, CEvent *pre):
	CEvent(engin, pre)
{
}

void CEventImplement::OnRead()
{
	LOG_FATAL("Have no implementation for CEvent::OnRead.");
	assert(false);
}

void CEventImplement::OnWrite()
{
	LOG_FATAL("Have no implementation for CEvent::OnWrite.");
	assert(false);
}

void CEventImplement::OnError()
{
	LOG_FATAL("Have no implementation for CEvent::OnError.");
	assert(false);
}

void CEventImplement::OnSubEvent(const CEvent*, ErrorCode)
{
	LOG_FATAL("Have no implementation for CEvent::OnSubEvent.");
	assert(false);
}


/** CTimer */

CTimer::~CTimer()
{
	Uninitialize();
}

inline bool CTimer::SetTimer(size_t value, size_t interval)
{
	if(value==0 && interval==0)
	{
		Uninitialize();

		return true;
	}

	if(true == Initialize())
	{
		itimerspec ts;

		ts.it_value.tv_sec = value / 1000000000;
		ts.it_value.tv_nsec = value % 1000000000; 
		ts.it_interval.tv_sec = interval / 1000000000;
		ts.it_interval.tv_nsec = interval % 1000000000;

		if(0 == timerfd_settime(m_Fd, 0, &ts, NULL))
			return true;
		else
			return false;
	}
	else
		return false;
}

inline bool CTimer::Initialize()
{
	if(m_Fd > 0)
		return true;
	else
	{
		m_Fd = timerfd_create(CLOCK_MONOTONIC, O_NONBLOCK);

		if(m_Fd > 0)
			return RegisterRD();
		else
			return false;
	}
}

void CTimer::Uninitialize()
{
	if(m_Fd > 0)
	{
		Unregister();
		close(m_Fd);
		m_Fd = -1;
	}
}

void CTimer::OnRead()
{
	if(m_Fd > 0)
	{
		size_t n;
		read(m_Fd, &n, sizeof n);
	}
	else
		assert(false);

	CEventEx *pre = dynamic_cast<CEventEx*>(m_Pre);
	if(pre)
		pre->OnTimer();
}


/** CEventEx */

CEventEx::CEventEx(CEventEngin *engin, CEvent *pre):
	CEvent(engin, pre)
{
}


/** CEventExImplement */

CEventExImplement::CEventExImplement(CEventEngin *engin, CEvent *pre):
	CEventEx(engin, pre), m_Timer(engin, this) 
{
}

bool CEventExImplement::SetTimer(size_t value, size_t interval)
{
	return m_Timer.SetTimer(value*1000000, interval*1000000);
}

void CEventExImplement::OnRead()
{
	LOG_FATAL("Have no implementation for CEventEx::OnRead.");
	assert(false);
}

void CEventExImplement::OnWrite()
{
	LOG_FATAL("Have no implementation for CEventEx::OnWrite.");
	assert(false);
}

void CEventExImplement::OnError()
{
	LOG_FATAL("Have no implementation for CEventEx::OnError.");
	assert(false);
}

void CEventExImplement::OnSubEvent(const CEvent*, ErrorCode)
{
	LOG_FATAL("Have no implementation for CEventEx::OnSubEvent.");
	assert(false);
}

void CEventExImplement::OnTimer()
{
	LOG_FATAL("Have no implementation for CEventEx::OnTimer.");
	assert(false);
}


/** CEventEngin */

CEventEngin::CEventEngin():
	m_Fd(-1), m_Priority(NULL), m_Doing(false)
{
}

CEventEngin::~CEventEngin()
{
	Uninitialize();
}

bool CEventEngin::Initialize(ssize_t affinity)
{
	m_Fd = epoll_create(MAX_EVENTS);
	if(m_Fd < 0)
	{
		LOG_FATAL("Create epoll failed(" << strerror(errno) << ").");

		return false;
	}

	if(false == Run())
	{
		close(m_Fd);
		m_Fd = -1;

		return false;
	}
	else
	{
		return SetAffinity(affinity);
	}
}

bool CEventEngin::AddPriority(CEvent *event)
{
	if(true == Add(event, ETREAD))
	{
		m_Priority = event;

		return true;
	}
	else
		return false;
}

bool CEventEngin::Add(CEvent *event, int tag)
{
	struct epoll_event et = {0};

	if(tag & ETREAD)
		et.events |= EPOLLIN;
	if(tag & ETWRITE)
		et.events |= EPOLLOUT;
	if(tag & ETET)
		et.events |= EPOLLET;
	et.data.ptr = (void*)event;

	if(0 == epoll_ctl(m_Fd, EPOLL_CTL_ADD, event->GetFd(), &et))
		return true;
	else
	{
		LOG_ERROR("Failed to add fd " << event->GetFd() << " into epoll(" << strerror(errno) << ").");

		return false;
	}
}

bool CEventEngin::Mod(CEvent *event, int tag)
{
	struct epoll_event et = {0};

	if(tag & ETREAD)
		et.events |= EPOLLIN;
	if(tag & ETWRITE)
		et.events |= EPOLLOUT;
	if(tag & ETET)
		et.events |= EPOLLET;
	et.data.ptr = (void*)event;

	if(0 == epoll_ctl(m_Fd, EPOLL_CTL_MOD, event->GetFd(), &et))
		return true;
	else
	{
		LOG_ERROR("Failed to modify fd " << event->GetFd() << " in epoll(" << strerror(errno) << ").");

		return false;
	}
}

bool CEventEngin::Del(CEvent *event)
{
	struct epoll_event et = {0};

	if(m_Doing == true)
		m_Delete.push_back(event);

	if(0 == epoll_ctl(m_Fd, EPOLL_CTL_DEL, event->GetFd(), &et))
		return true;
	else
	{
		LOG_ERROR("Failed to delete fd " << event->GetFd() << " from epoll(" << strerror(errno) << ").");

		return false;
	}
}

bool CEventEngin::Uninitialize()
{
	Stop();

	if(m_Fd > 0)
	{
		close(m_Fd);
		m_Fd = -1;
	}

	return true;
}

void CEventEngin::Thread()
{
	int n = epoll_wait(m_Fd, m_Events, MAX_EVENTS, 1000);
	CEvent *event;
	int priority;

	m_Doing = true;

	for(priority=0; priority<n; priority++) //For priority.
	{
		event = static_cast<CEvent*>(m_Events[priority].data.ptr);
		if(m_Priority == event)
		{
			//LOG_TRACE("Get a new epoll event for priority(" << m_Events[priority].data.ptr << ":" << m_Events[priority].events << ").");
			if(m_Events[priority].events == EPOLLIN)
			{
				event->OnRead();
				break;
			}
			else
				assert(false);
		}
	}

	for(int i=0; i<n; i++)
	{
		if(i == priority) //Skip priority.
			continue;

		event = static_cast<CEvent*>(m_Events[i].data.ptr);

		if(m_Delete.end() != find(m_Delete.begin(), m_Delete.end(), event))
			continue;

		//LOG_TRACE("Get a new epoll event(" << m_Events[i].data.ptr << ":" << m_Events[i].events << ").");

		if(m_Events[i].events & EPOLLERR)
		{
			event->OnError();
			continue;
		}

		if(m_Events[i].events & EPOLLHUP)
		{
			event->OnError();
			continue;
		}

		if(m_Events[i].events & EPOLLIN)
			event->OnRead();

		if(m_Events[i].events & EPOLLOUT)
			event->OnWrite();
	}

	m_Delete.clear();
	m_Doing = false;

	if(n < 0)
		switch(errno)
		{
		case EINTR:
			break;
		default:
			LOG_ERROR("Failed to wait for epoll event(" << strerror(errno) << ").");
		}
}
