#ifndef RTSPSESSION_H
#define RTSPSESSION_H


#include "Socket.h"
#include "Event.h"
#include "Http.h"
#include "Mp4Player.h"


class CRtspSession: public CEventImplement
{
public:
	CRtspSession(CEventEngin*, CEvent*);
	virtual ~CRtspSession();
	bool Initialize();
	bool Handle(int fd);
private:
	void ProcessRequest();
	void OnDescribe();
	bool OnSetup();
	void OnPlay();
	void SendError(ErrorCode);
	void Close();
	/** CEventImplement */
	int GetFd() {return m_Connect.GetFd();}
	void OnRead();
	void OnWrite();
private:
	CTcp m_Connect;
	CHttpRequest m_Request;
	CHttpResponse m_Response;
	CMp4Player m_Mp4;
	size_t m_Type; //RTSP=1 or RTCP=2
	size_t m_RTCP; //Length of RTCP packet
	size_t m_HeaderSended;
	string m_Body;
	size_t m_BodySended;
};


#endif
