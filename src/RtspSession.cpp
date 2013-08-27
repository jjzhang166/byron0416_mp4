#include <sstream>
#include "stdlib.h"
#include "RtspSession.h"


CRtspSession::CRtspSession(CEventEngin *engin, CEvent *pre):
	CEventImplement(engin, pre)
{
	//m_Body = "v=0\r\no=OnewaveUServerNG 1451516402 1025358037 IN IP4 192.168.20.136\r\ns=/xxx666\r\nu=http:///\r\ne=admin@\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=isma-compliance:1,1.0,1\r\na=range:npt=0-\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 MP4V-ES/90000\r\na=fmtp:96 profile-level-id=245;config=000001B0F5000001B509000001000000012000C888B0E0E0FA62D089028307\r\na=control:trackID=0\r\n";
	m_Body = "v=0\r\no=OnewaveUServerNG 1451516402 1025358037 IN IP4 0.0.0.0\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=range:npt=0-1000.9\r\nm=video 0 TCP/AVP 96\r\na=rtpmap:96 H264\r\na=control:trackID=0\r\n";
	//m_Body = "v=0\r\no=OnewaveUServerNG 1451516402 1025358037 IN IP4 192.168.0.101\r\ns=/xxx666\r\nu=http:///\r\nt=0 0\r\na=range:npt=0-72.080000\r\nm=video 1234 RTP/AVP 96\r\na=rtpmap:96 H264\r\nc=IN IP4 192.168.0.101\r\n";
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
	m_Socket.Attach(fd);
	RegisterRD();

	return true;
}

void CRtspSession::OnRead()
{
	char buf[2048];
	m_HeaderSended = m_BodySended = 0;

	size_t len = m_Socket.Recv(buf, 2048);
	m_Buf += buf;

	if(m_Request.Parse(m_Buf) == 200)
	{
		if(m_Request.GetMethod() == "OPTIONS")
		{
			string name;
			string value;

			//Cseq
			name = "CSeq";
			m_Request.GetField(name, value);
			m_Response.AddField(name, value);
			//Public
			name = "Public";
			value = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, SCALE, GET_PARAMETER";
			m_Response.AddField(name, value);
			//Server
			name = "Server";
			value = "SS";
			m_Response.AddField(name, value);

			m_Header = m_Response.Response(200);

			RegisterWR();
		}
		else if(m_Request.GetMethod() == "DESCRIBE")
		{
			string name;
			string value;

			//Cseq
			name = "CSeq";
			m_Request.GetField(name, value);
			m_Response.AddField(name, value);
			//Server
			name = "Server";
			value = "SS";
			m_Response.AddField(name, value);
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

			m_Header = m_Response.Response(200);

			RegisterWR();
		}
		else if(m_Request.GetMethod() == "SETUP")
		{
			string name;
			string value;

			name = "CSeq";
			m_Request.GetField(name, value);
			m_Response.AddField(name, value);
			//Server
			name = "Server";
			value = "SS";
			m_Response.AddField(name, value);

			m_Header = m_Response.Response(200);

			RegisterWR();
		}
	}
}

void CRtspSession::OnWrite()
{
	size_t len;

	while(true)
	{
		if(m_HeaderSended < m_Header.size())
		{
			len = m_Socket.Send(m_Header.data()+m_HeaderSended, m_Header.size()-m_HeaderSended);
			if(len > 0)
				m_HeaderSended += len;
		}
		else
		{
			if(m_BodySended < m_Body.size())
			{
				len = m_Socket.Send(m_Body.data()+m_BodySended, m_Body.size()-m_BodySended);
				if(len > 0)
					m_BodySended += len;
			}
			else
				break;
		}
	}

	RegisterRD();
}
