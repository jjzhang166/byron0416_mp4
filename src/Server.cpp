#include <sstream>
#include <errno.h>
#include <string.h>
#include "HttpSession.h"
#include "Server.h"


using std::ostringstream;

/** CEngin */

CMutex CEngin::m_Mutex;

CEngin::CEngin():
	CEventExImplement(&m_Engin, NULL), m_SessionCount(100)
{
	m_Free = m_SessionCount;
	m_Done = 0;
}

CEngin::~CEngin()
{
	m_Engin.Stop();

	SetTimer(0, 0);
	Unregister();

	for(list<CSession*>::iterator iter=m_Sessions.begin(); iter!=m_Sessions.end(); iter++)
		delete *iter;

	m_Engin.Uninitialize();

	m_Listener.Detach(); //Void to close fd of listener.
}

bool CEngin::Run(int listener, size_t affinity)
{
	for(size_t i=0; i<m_SessionCount; i++)
	{
		CSession *session = new CHttpSession(&m_Engin, this);
		if(true == session->Initialize())
		{
			session->SetLog(m_Log);
			m_Sessions.push_back(session);
		}
		else
		{
			LOG_FATAL("Failed to initialize session.");
			delete session;

			return false;
		}
	}

	if(false == m_Engin.Initialize(affinity))
	{
		LOG_FATAL("Failed to start CEventEngin.");

		return false;
	}

	if(false == m_Listener.Attach(listener))
	{
		LOG_FATAL("Failed to attach listener fd.");

		return false;
	}

	if(false == SetTimer(50, 5))
	{
		LOG_FATAL("Failed to set timer for hls engin.");

		return false;
	}

	{ // For title.
		ostringstream out;

		out << affinity;
		m_Title = out.str();
	}

	return true;
}

void CEngin::SetLog(CLog *log)
{
	m_Log = log;

	m_Engin.SetLog(log);
	for(list<CSession*>::iterator iter=m_Sessions.begin(); iter!=m_Sessions.end(); iter++)
		(*iter)->SetLog(m_Log);
}

void CEngin::OnRead()
{
	list<int> fds;

	while(m_Free > 0)
	{
		int fd = m_Listener.Accept();
		if(fd > 0)
		{
			fds.push_back(fd);
			m_Free--;
		}
		else if(errno == EAGAIN)
			break;
		else
		{
			LOG_ERROR("Failed to accept new request(" << strerror(errno) << ").");
			break;
		}
	}

	m_Mutex.Unlock();
	SetTimer(40, 5);
	Unregister();

	for(list<int>::iterator iter=fds.begin(); iter!=fds.end(); iter++)
	{
		int fd = *iter;

		CSession *session = m_Sessions.front();
		m_Sessions.pop_front();

		if(session->AcceptRequest(*iter) == true)
		{
			ostringstream out;

			LOG_INFO("Accept a connect(" << fd << ") with session " << m_Done << ".");

			out << m_Done << "@" << m_Title;
			session->SetTitle(out.str());
			m_Done ++;
		}
		else
		{
			LOG_ERROR("Failed to handle new socket(" << fd << ").");

			close(fd);
			m_Sessions.push_back(session);
			m_Free++;
		}
	}
}

void CEngin::OnSubEvent(const CEvent *event, ErrorCode err)
{
	CEvent *ev = const_cast<CEvent*>(event);
	CSession *session = dynamic_cast<CSession*>(ev);

	m_Sessions.push_back(session);
	m_Free++;
	assert(m_Free <= m_SessionCount);
}

void CEngin::OnTimer()
{
	if(m_Free > 0)
	{
		if(true == m_Mutex.TryLock())
		{
			if(RegisterPriority() == true)
			{
				LOG_DEBUG("Change to #" << m_Title << " hls engin.");
				if(false == SetTimer(0, 0))
					LOG_ERROR("Failed to close timer for hls engin.");
			}
			else
				m_Mutex.Unlock();
		}
	}
}


/** CServer */

CServer::CServer():
	CEventExImplement(&m_Engin, NULL)
{
}

CServer::~CServer()
{
	m_Engin.Stop();

	SetTimer(0, 0);

	for(size_t i=0; i<m_Engins.size(); i++)
		delete m_Engins[i];

	m_Listener.Close();

	m_Engin.Uninitialize();
}

bool CServer::Run(size_t port, const string &ip)
{
	if(m_Listener.Listen(port, ip) == false)
	{
		LOG_FATAL("Failed to run stream server at " << ip << ":" << port <<".");

		return false;
	}

	m_EnginCount = sysconf(_SC_NPROCESSORS_ONLN);
	for(size_t i=0; i<m_EnginCount; i++)
	{
		CEngin *engin = new CEngin();
		if(engin)
		{
			engin->SetLog(m_Log);
			if(true == engin->Run(m_Listener.GetFd(), i))
				m_Engins.push_back(engin);
			else
			{
				delete engin;

				return false;
			}
		}
		else
			return false;
	}

	m_Engin.Initialize();
	SetTimer(5000, 5000);

	return true;
}

void CServer::SetLog(CLog *log)
{
	m_Log = log;

	m_Engin.SetLog(log);
	m_Listener.SetLog(log);
	for(vector<CEngin*>::iterator iter=m_Engins.begin(); iter!=m_Engins.end(); iter++)
		(*iter)->SetLog(log);
}

void CServer::OnTimer()
{
	ostringstream out;

	out << "Server status:";
	for(size_t i=0; i<m_Engins.size(); i++)
		out << " " <<  m_Engins[i]->GetFree() << "(" << m_Engins[i]->GetDone() << ")";

	LOG_TRACE(out.str());
}
