#ifndef LIVEPLAYER_H
#define LIVEPLAYER_H


#include <map>
#include "Event.h"
#include "Socket.h"
#include "Rtp.h"


using std::map;

class CLiveTrack: public CEventImplement
{
public:
	CLiveTrack(CEventEngin*);
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
	bool Seek(size_t) {return true;}
	bool Play(int);
	size_t GetCurrentPos() {return 0;}
	bool Pause();
	bool Resume();
	void Teardown();
	//CLogger
	void SetLog(CLog*);
	void SetTitle(const string&);
private:
	/** CEventExImplement */
	int GetFd() {return -1;}
private:
	CTcp m_Rtsp;
	string m_Name;
	string m_Sdp;
	map<size_t, size_t> m_Interleaved;
	vector<CLiveTrack> m_Tracks;
};


#endif
