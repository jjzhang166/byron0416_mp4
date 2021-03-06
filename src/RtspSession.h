#ifndef RTSPSESSION_H
#define RTSPSESSION_H


#include "Socket.h"
#include "Event.h"
#include "Http.h"
#include "Rtcp.h"
#include "Mp4Player.h"
#include "LivePlayer.h"


class CRtspSession: public CEventImplement
{
public:
	CRtspSession(CEventEngin*, CEvent*);
	virtual ~CRtspSession();
	bool Initialize();
	bool Handle(int fd);
	//CLogger
	void SetLog(CLog*);
	void SetTitle(const string&);
private:
	string GetSessionID();
	void ProcessRequest();
	ErrorCode OnDescribe();
	ErrorCode OnSetup();
	ErrorCode OnPlay();
	ErrorCode OnPause();
	void SendError(ErrorCode);
	void Close();
	/** CEventImplement */
	int GetFd() {return m_Connect.GetFd();}
	void OnRead();
	void OnWrite();
	void OnError();
private:
	CTcp m_Connect;
	CHttpRequest m_Request;
	CHttpResponse m_Response;
	CRtcp m_Rtcp;
	CMp4Player m_Mp4;
	CLivePlayer m_Live;
	CRtpPlayer *m_Player;
	bool m_Pause;
	size_t m_Type; //RTSP=1 or RTCP=2
	size_t m_HeaderSended;
	string m_Body;
	size_t m_BodySended;
	static size_t m_Session;
	static CMutex m_Mutex;
};


#endif
