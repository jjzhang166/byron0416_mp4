//#include <sstream>
//#include "stdlib.h"
#include <errno.h>
#include "RtspSession.h"


CRtspSession::CRtspSession(CEventEngin *engin, CEvent *pre):
	CEventImplement(engin, pre), m_Mp4(engin, this)
{
	//ALL//m_Body = "v=0\r\no=- 15404613757824320910 15404613757824320910 IN IP4 Byron-PC\r\ns=Unnamed\r\ni=N/A\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.7\r\na=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\na=control:rtsp://192.168.0.100:8554/aaaa\r\nm=audio 0 RTP/AVP 96\r\nb=RR:0\r\na=rtpmap:96 mpeg4-generic/8000\r\na=fmtp:96 streamtype=5; profile-level-id=15; mode=AAC-hbr; config=1588; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1;\r\na=control:rtsp://192.168.0.100:8554/aaaa/trackID=0\r\nm=video 0 RTP/AVP 96\r\nb=RR:0\r\na=rtpmap:96 MP4V-ES/90000\r\na=fmtp:96 profile-level-id=3; config=000001b001000001b50ee040c0cf0000010000000120008440fa28282078a21f;\r\na=control:rtsp://192.168.0.100:8554/aaaa/trackID=1\r\n";
	m_Body = "v=0\r\no=- 15404613757824320910 15404613757824320910 IN IP4 Byron-PC\r\ns=Unnamed\r\ni=N/A\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.7\r\na=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\na=control:rtsp://192.168.0.100:8554/aaaa\r\nm=video 0 RTP/AVP 96\r\nb=RR:0\r\na=rtpmap:96 MP4V-ES/90000\r\na=fmtp:96 profile-level-id=3; config=000001b001000001b50ee040c0cf0000010000000120008440fa28282078a21f;\r\na=control:rtsp://192.168.0.100:8554/aaaa/trackID=1\r\n";
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
		value = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, SCALE, GET_PARAMETER";
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
		value = "d3d8b954dcd7d072";
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

	m_Request.Initialize();

	m_Response.Response("RTSP/1.0", E_OK);
	RegisterWR();
}

void CRtspSession::OnDescribe()
{
	vector<size_t> ids;

	m_Mp4.Setup("b.mp4");
	m_Mp4.GetTrackID(ids);
	m_Mp4.GetSdp(ids[0], m_Body);
}

void CRtspSession::OnSetup()
{
}

void CRtspSession::OnPlay()
{
}

void CRtspSession::SendError(ErrorCode)
{
}

void CRtspSession::Close()
{
}

void CRtspSession::OnRead()
{
	while(true)
	{
		const ssize_t LEN = 1024;
		char buf[LEN+1] = {0};
		ssize_t len = m_Connect.Recv(buf, LEN);
		if(len > 0)
		{
			if(buf[0] == '$') /** RTCP */
				return;
			else
			{
				ErrorCode code = m_Request.Parse(buf);
				if(code == E_OK)
				{
					LOG_INFO("Get a rtsp request: " << m_Request.GetRequest());
					return ProcessRequest();
				}
				else if(code == E_CONTINUE)
					continue;
				else
					return SendError(code);
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
