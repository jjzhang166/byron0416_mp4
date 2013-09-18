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
	/** CRtpPlayer */
	bool Setup(const string&);
	bool GetTrackID(vector<size_t>&);
	bool GetSdp(size_t, string&);
	bool SetInterleaved(size_t, size_t);
	bool Play(int);
	void Teardown();
private:
	/** CEventExImplement */
	int GetFd() {return -1;}
	void OnTimer();
private:
	CMp4Demuxer m_Mp4;
	CTcp m_Rtsp;
	size_t m_StartTime;
	map<size_t, size_t> m_Interleaved;
	map<size_t, CRtpSample> m_Samples;
};


#endif
