#include <sstream>
#include "stdlib.h"
#include "RtspSession.h"


CRtspSession::CRtspSession(CEventEngin *engin, CEvent *pre):
	CEventImplement(engin, pre), m_Mp4(engin, this)
{
	//m_Body = "v=0\r\no=OnewaveUServerNG 1451516402 1025358037 IN IP4 192.168.20.136\r\ns=/xxx666\r\nu=http:///\r\ne=admin@\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=isma-compliance:1,1.0,1\r\na=range:npt=0-\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 MP4V-ES/90000\r\na=fmtp:96 profile-level-id=245;config=000001B0F5000001B509000001000000012000C888B0E0E0FA62D089028307\r\na=control:trackID=0\r\n";
	//m_Body = "v=0\r\no=OnewaveUServerNG 1451516402 1025358037 IN IP4 0.0.0.0\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=range:npt=0-1000.9\r\nm=video 0 TCP/AVP 96\r\na=rtpmap:96 H264\r\na=control:trackID=0\r\n";
	//m_Body = "v=0\r\no=OnewaveUServerNG 1451516402 1025358037 IN IP4 192.168.0.101\r\ns=/xxx666\r\nu=http:///\r\nt=0 0\r\na=range:npt=0-72.080000\r\nm=video 1234 RTP/AVP 96\r\na=rtpmap:96 H264\r\nc=IN IP4 192.168.0.101\r\n";

	//m_Body = "m=video 1234 RTP/AVP 96\r\nb=AS:30\r\na=rtpmap:96 MP4V-ES/90000\r\na=control:trackID=3\r\na=cliprect:0,0,120,160\r\na=fmtp:96 profile-level-id=1;config=000001B001000001B50EE040C0CF0000010000000120008440FA28282078A21F\r\na=mpeg4-esid:201\r\n";
	//
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
	m_Socket.Attach(fd);
	RegisterRD();

	return true;
}

void CRtspSession::OnRead()
{
	char buf[2048] = {0};
	m_HeaderSended = 0;
	m_BodySended = 0;

	size_t len = m_Socket.Recv(buf, 2048);
	if(buf[0] == '$')
		return;
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
			//Length
			name = "Content-Length";
			value = "0";
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

			m_BodySended = m_Body.size();
			RegisterWR();
		}
		else if(m_Request.GetMethod() == "DESCRIBE")
		{
			string name;
			string value;

			//Server
			name = "Server";
			value = "SS";
			m_Response.AddField(name, value);
			//Cseq
			name = "CSeq";
			m_Request.GetField(name, value);
			m_Response.AddField(name, value);
			//Date
			name = "Date";
			value = "Wed, 28 Aug 2013 08:08:56 GMT";
			m_Response.AddField(name, value);
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

			m_Header = m_Response.Response(200);

			m_BodySended = 0;
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
			//Session
			name = "Session";
			value = "d3d8b954dcd7d072";
			m_Response.AddField(name, value);
			//Transport
			name = "Transport";
			m_Request.GetField(name, value);
			//value = "RTP/AVP/TCP;unicast;interleaved=0-1";
			m_Response.AddField(name, value);

			m_Header = m_Response.Response(200);

			m_BodySended = m_Body.size();
			RegisterWR();

			const string path = "b.mp4";
			m_Mp4.Setup(path);
		}
		else if(m_Request.GetMethod() == "PLAY")
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
			//Session
			name = "Session";
			value = "d3d8b954dcd7d072";
			m_Response.AddField(name, value);

			m_Header = m_Response.Response(200);

			m_BodySended = m_Body.size();
			RegisterWR();

			m_Mp4.Play(GetFd());
		}
		else if(m_Request.GetMethod() == "GET_PARAMETER")
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
			//Session
			name = "Session";
			value = "d3d8b954dcd7d072";
			m_Response.AddField(name, value);

			m_Header = m_Response.Response(200);

			m_BodySended = m_Body.size();
			RegisterWR();
		}
		m_Buf = "";
	}
}

void CRtspSession::OnWrite()
{
	size_t len;

	while(true)
	{
		if(m_HeaderSended < m_Header.size())
		{
			LOG_INFO("Send a response:\n" << m_Header);
			len = m_Socket.Send(m_Header.data()+m_HeaderSended, m_Header.size()-m_HeaderSended);
			if(len > 0)
				m_HeaderSended += len;
		}
		else
		{
			if(m_BodySended < m_Body.size())
			{
				LOG_INFO("Send a body:\n" << m_Body);
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
