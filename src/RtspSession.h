#ifndef RTSPSESSION_H
#define RTSPSESSION_H


#include "Socket.h"
#include "Event.h"
#include "Rtsp.h"
#include "Mp4Player.h"


class CRtspSession: public CEventImplement
{
public:
	CRtspSession(CEventEngin*, CEvent*);
	virtual ~CRtspSession();
	bool Initialize();
	virtual bool Handle(int fd);
	int GetFd() {return m_Socket.GetFd();}
	void OnRead();
	void OnWrite();
private:
	CTcp m_Socket;
	CRtspRequest m_Request;
	CRtspResponse m_Response;
	string m_Buf;
	string m_Header;
	size_t m_HeaderSended;
	string m_Body;
	size_t m_BodySended;
	CMp4Player m_Mp4;
};


#endif
