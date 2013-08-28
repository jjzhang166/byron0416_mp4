#ifndef MP4PLAYER_H
#define MP4PLAYER_H


#include "Event.h"
#include "Socket.h"
#include "Rtp.h"
#include "Mp4.h"


class CMp4Player: public CRtpPlayer, public CEventExImplement
{
public:
	CMp4Player(CEventEngin*, CEvent*);
	bool Setup(const string&);
	bool Play(int);
private:
	int GetFd() {return -1;}
	void OnTimer();
private:
	CMp4Demuxer m_Mp4;
	CTcp m_Udp;
	size_t m_StartTime;
	map<size_t, CRtpSample> m_Samples;
};


#endif
