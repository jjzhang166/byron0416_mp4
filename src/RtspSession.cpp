#include <arpa/inet.h> //ntohs
#include <sstream>
#include <errno.h>
#include "RtspSession.h"


using std::istringstream;
using std::ostringstream;

/** CRtspSession */

size_t CRtspSession::m_Session = 0x1111111111111110;
CMutex CRtspSession::m_Mutex;

CRtspSession::CRtspSession(CEventEngin *engin, CEvent *pre):
	CEventImplement(engin, pre), m_Mp4(engin, this), m_Live(engin, this), m_Pause(false), m_Type(0)
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
	m_Connect.SetNoDelay();
	RegisterRD();

	return true;
}

void CRtspSession::SetLog(CLog *log)
{
	m_Log = log;

	m_Request.SetLog(m_Log);
	m_Response.SetLog(m_Log);
	m_Mp4.SetLog(m_Log);
	m_Live.SetLog(m_Log);
}

void CRtspSession::SetTitle(const string &title)
{
	m_Title = title;

	m_Request.SetTitle(m_Title);
	m_Response.SetTitle(m_Title);
	m_Mp4.SetTitle(m_Title);
	m_Live.SetTitle(m_Title);
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

void CRtspSession::ProcessRequest()
{
	string name, value;
	ErrorCode ret = E_OK;

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
		ret = OnDescribe();

		//Type
		name = "Content-Type";
		value = "application/sdp";
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
		ret = OnSetup();

		//Session
		name = "Session";
		m_Request.GetField(name, value);
		if(value.empty())
			value = GetSessionID();
		m_Response.AddField(name, value);

		//Transport
		if(ret == E_OK)
		{
			name = "Transport";
			m_Request.GetField(name, value);
			m_Response.AddField(name, value);
		}
	}
	else if(m_Request.GetMethod() == "PLAY")
	{
		ret = OnPlay();

		//Session
		name = "Session";
		m_Request.GetField(name, value);
		m_Response.AddField(name, value);

		//Range
		name = "Range";
		m_Request.GetField(name, value);
		if(!value.empty())
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
		OnPause();

		//Session
		name = "Session";
		m_Request.GetField(name, value);
		m_Response.AddField(name, value);
	}
	else if(m_Request.GetMethod() == "TEARDOWN")
	{
		if(m_Player)
			m_Player->Teardown();

		//Session
		name = "Session";
		m_Request.GetField(name, value);
		m_Response.AddField(name, value);
	}

	m_Request.Initialize();

	m_Response.Response("RTSP/1.0", ret);
	RegisterWR();
}

ErrorCode CRtspSession::OnDescribe()
{
	vector<size_t> ids;

	if(true == m_Live.Setup(m_Request.GetFile()))
		m_Player = &m_Live;
	else if(true == m_Mp4.Setup(m_Request.GetFile()))
		m_Player = &m_Mp4;
	else
		return E_NOTFOUND;

	m_Body = m_Player->GetSdp();

	return E_OK;
}

ErrorCode CRtspSession::OnSetup()
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
				m_Player->SetInterleaved(id, interleaved);
			}
			else
				return E_UNSUPPORTEDTRANSPORT;
		}

		return E_OK;
	}
	else
		return E_UNSUPPORTEDTRANSPORT;
}

ErrorCode CRtspSession::OnPlay()
{
	string name, value;

	name = "Range";
	m_Request.GetField(name, value);
	if(value.empty())
	{
		size_t pos = m_Player->GetCurrentPos();
		m_Player->Seek(pos);
	}
	else
	{
		value = value.substr(value.find("npt=")+4, string::npos);
		float pos;
		istringstream in(value);

		in >> pos;
		LOG_ERROR("xxxxxxxxxxxxxxxxxxxx " << pos);
		m_Player->Seek((size_t)(pos*1000));
	}

	if(true == m_Player->Play(GetFd()))
		return E_OK;
	else
		return E_ERROR;

	/*
	if(m_Pause)
	{
		m_Player->Resume();

		return E_OK;
	}
	else
	{
		if(true == m_Player->Play(GetFd()))
			return E_OK;
		else
			return E_ERROR;
	}
	*/
}

ErrorCode CRtspSession::OnPause()
{
	m_Player->Pause();
	m_Pause = true;

	return E_OK;
}

void CRtspSession::SendError(ErrorCode)
{
	Close();
}

void CRtspSession::Close()
{
	Unregister();
	m_Connect.Close();
	if(m_Player)
		m_Player->Teardown();

	ReturnEvent(E_OK);
}

void CRtspSession::OnRead()
{
	while(true)
	{
		const ssize_t LEN = 1024;
		char BUF[LEN+1] = {0};
		ssize_t len = m_Connect.Recv(BUF, LEN);
		if(len > 0)
		{
			char *buf = BUF;
			size_t l = len;
			while(l > 0)
			{
				if(m_Type == 0)
				{
					if(buf[0] != '$') /** RTSP */
						m_Type = 1;
					else /** RTCP */
					{
						m_Type = 2;
						buf += 2;
						l -= 2;
					}
				}
				else if(m_Type == 1) /** RTSP */
				{
					size_t n = l;

					ErrorCode code = m_Request.Parse(buf, n);
					l -= n;
					buf += n;

					if(code == E_OK)
					{
						m_Type = 0;
						LOG_INFO("Get a rtsp request: " << m_Request.GetRequest());
						ProcessRequest();
						continue;
					}
					else if(code == E_CONTINUE)
						break;
					else
						return SendError(code);
				}
				else if(m_Type == 2) /** RTCP */
				{
					size_t n = l;

					ErrorCode code = m_Rtcp.Parse(buf, l);
					l -= n;
					buf += n;

					if(code == E_OK)
					{
						m_Type = 0;
						continue;
					}
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
	ssize_t len;

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

		if(len == 0)
		{
			LOG_INFO("Socket is closed by client when writing data.");

			return Close();
		}
		else if(-1 == len)
		{
			if(errno == EAGAIN)
				return;
			else
			{
				LOG_WARN("Failed to send socket(" << errno << ").");

				return Close();
			}
		}
	}

	RegisterRD();
}

void CRtspSession::OnError()
{
	Close();
}
