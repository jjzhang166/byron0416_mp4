#include <arpa/inet.h> //ntohs
#include <sstream>
#include <errno.h>
#include "Thread.h"
#include "RtspSession.h"


using std::istringstream;
using std::ostringstream;

/** CRtspSession */

size_t CRtspSession::m_Session = 0x1111111111111111;
CMutex CRtspSession::m_Mutex;

CRtspSession::CRtspSession(CEventEngin *engin, CEvent *pre):
	CEventImplement(engin, pre), m_Mp4(engin, this)
{
}

CRtspSession::~CRtspSession()
{
}

bool CRtspSession::Initialize()
{
	return true;
}

bool CRtspSession::Handle(int fd)
{
	m_Connect.Attach(fd);
	RegisterRD();

	return true;
}

void CRtspSession::ProcessRequest()
{
	string name, value;

	m_Response.Initialize();
	m_HeaderSended = 0;
	m_Body = "";
	m_BodySended = 0;

	//Cseq
	name = "CSeq";
	m_Request.GetField(name, value);
	m_Response.AddField(name, value);

	//Length
	name = "Content-Length";
	value = "0";
	m_Response.AddField(name, value);

	//Server
	name = "Server";
	value = "SS";
	m_Response.AddField(name, value);

	//Date
	//name = "Date";
	//value = "Wed, 28 Aug 2013 08:08:56 GMT";
	//m_Response.AddField(name, value);

	if(m_Request.GetMethod() == "OPTIONS")
	{
		//Public
		name = "Public";
		value = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER";
		m_Response.AddField(name, value);
	}
	else if(m_Request.GetMethod() == "DESCRIBE")
	{
		OnDescribe();

		//Type
		name = "Content-Type";
		value = "application/sdp";
		m_Response.AddField(name, value);

		//Base
		name = "Content-Base";
		value = "rtsp://192.168.0.101:8554/aaaa";
		m_Response.AddField(name, value);

		//Length
		std::ostringstream o;
		o << m_Body.size();
		name = "Content-Length";
		value = o.str();
		m_Response.AddField(name, value);
	}
	else if(m_Request.GetMethod() == "SETUP")
	{
		OnSetup();

		//Session
		name = "Session";
		m_Request.GetField(name, value);
		if(value.empty())
			value = GetSessionID();
		m_Response.AddField(name, value);

		//Transport
		name = "Transport";
		m_Request.GetField(name, value);
		m_Response.AddField(name, value);
	}
	else if(m_Request.GetMethod() == "PLAY")
	{
		OnPlay();

		//Session
		name = "Session";
		m_Request.GetField(name, value);
		m_Response.AddField(name, value);
	}
	else if(m_Request.GetMethod() == "GET_PARAMETER")
	{
		//Session
		name = "Session";
		m_Request.GetField(name, value);
		m_Response.AddField(name, value);
	}
	else if(m_Request.GetMethod() == "PAUSE")
	{
		assert(false);
	}

	m_Request.Initialize();

	m_Response.Response("RTSP/1.0", E_OK);
	RegisterWR();
}

void CRtspSession::OnDescribe()
{
	vector<size_t> ids;
	string sdp;

	m_Mp4.Setup(m_Request.GetFile());
	m_Mp4.GetTrackID(ids);
	for(size_t i=0; i<ids.size(); i++)
	{
		m_Mp4.GetSdp(ids[i], sdp);
		m_Body += sdp;
	}
}

bool CRtspSession::OnSetup()
{
	size_t id;

	/** find track */
	string track = m_Request.GetFile();
	track.substr(track.rfind("/")+1, string::npos);

	/** find id */
	size_t pos = track.find("trackID=");
	if(pos != string::npos)
	{
		istringstream in(track.substr(pos+8, string::npos));
		in >> id;

		/** interleaved */
		{
			string transport;
			size_t interleaved;
			size_t pos;

			m_Request.GetField("Transport", transport);
			pos = transport.find("interleaved=");
			if(pos != string::npos)
			{
				istringstream num(transport.substr(pos+12, string::npos));
				num >> interleaved;
				m_Mp4.SetInterleaved(id, interleaved);
			}
			else
				return false;

		}

		return true;
	}
	else
		return false;
}

void CRtspSession::OnPlay()
{
	assert(m_Mp4.Play(GetFd()));
}

void CRtspSession::SendError(ErrorCode)
{
}

void CRtspSession::Close()
{
}

string CRtspSession::GetSessionID()
{
	{
		CAutoMutex mutex(&m_Mutex);
		m_Session ++;
	}

	ostringstream out;
	out << std::hex << m_Session;

	return out.str();
}

void CRtspSession::OnRead()
{
	while(true)
	{
		const ssize_t LEN = 1024;
		char BUF[LEN+1] = {0};
		char *buf = BUF;
		ssize_t len = m_Connect.Recv(buf, LEN);
		if(len > 0)
		{
			while(len > 0)
			{
				if(m_Type == 0)
				{
					if(buf[0] != '$') /** RTCP */
					{
						m_Type = 1;

					}
					else
					{
						m_Type = 2;
						buf += 2;
						len -= 2;
					}
				}

				if(m_Type == 1)
				{
					ErrorCode code = m_Request.Parse(buf, len);
					if(code == E_OK)
					{
						LOG_INFO("Get a rtsp request: " << m_Request.GetRequest());
						ProcessRequest();
						continue;
					}
					else if(code == E_CONTINUE)
						break;
					else
						return SendError(code);
				}

				if(m_Type == 2)
				{
					ErrorCode code = m_Rtcp.Parse(buf, len);
					if(code == E_OK)
						continue;
					else if(code == E_CONTINUE)
						break; 
					else
						return SendError(code);
				}

			}
		}
		else if(len == 0)
		{
			LOG_INFO("Socket is closed by client when reading data.");

			return Close();
		}
		else if(-1 == len)
		{
			if(errno == EAGAIN)
				return;
			else
			{
				LOG_WARN("Failed to recieve socket(" << errno << ").");

				return Close();
			}
		}
	}
}

void CRtspSession::OnWrite()
{
	size_t len;

	while(true)
	{
		if(m_HeaderSended == 0)
			LOG_INFO("Send a response:\n" << m_Response.GetHeader());
		if(m_HeaderSended < m_Response.GetHeader().size())
		{
			len = m_Connect.Send(m_Response.GetHeader().data()+m_HeaderSended, m_Response.GetHeader().size()-m_HeaderSended);
			if(len > 0)
				m_HeaderSended += len;
		}
		else
		{
			if(m_BodySended < m_Body.size())
			{
				LOG_INFO("Send a body:\n" << m_Body);
				len = m_Connect.Send(m_Body.data()+m_BodySended, m_Body.size()-m_BodySended);
				if(len > 0)
					m_BodySended += len;
			}
			else
				break;
		}
	}

	RegisterRD();
}

void CRtspSession::OnError()
{
	Close();
}
