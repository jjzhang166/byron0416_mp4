#ifndef LIVEPLAYER_H
#define LIVEPLAYER_H


#include <map>
#include "Event.h"
#include "Socket.h"
#include "Rtp.h"


using std::map;

class CTrack: public CEventImplement
{
public:
	CTrack(CEventEngin*);
	bool SetFd(int fd) {return m_Rtsp.Attach(fd);}
	void SetPort(size_t port) {m_Port = port;}
	void SetID(size_t id) {m_ID = id;}
	size_t GetID() {return m_ID;}
	void SetInterleaved(size_t interleaved) {m_Interleaved = interleaved;}
	bool Run();
	void Stop();
private:
	int GetFd() {return m_Server.GetFd();}
	void OnRead();
private:
	CTcp m_Rtsp;
	CUdpServer m_Server;
	size_t m_Port;
	size_t m_ID;
	char m_Interleaved;
};

class CLivePlayer: public CRtpPlayer, public CEventImplement
{
public:
	CLivePlayer(CEventEngin*, CEvent*);
	/** CRtpPlayer */
	bool Setup(const string&);
	bool GetTrackID(vector<size_t>&);
	string GetSdp();
	bool SetInterleaved(size_t, size_t);
	bool Play(int);
	void Teardown();
private:
	/** CEventExImplement */
	int GetFd() {return -1;}
private:
	CTcp m_Rtsp;
	string m_Name;
	string m_Sdp;
	map<size_t, size_t> m_Interleaved;
	vector<CTrack> m_Tracks;
};


#endif
