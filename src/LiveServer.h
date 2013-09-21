#ifndef LIVESERVER_H
#define LIVESERVER_H


#include <map>
#include "Event.h"
#include "Socket.h"


using std::map;

class CLiveTrack: public CEventImplement
{
public:
	CLiveTrack(CEventEngin*);
	bool Run(size_t);
	bool Stop();
private:
	int GetFd() {return m_Server.GetFd();}
	void OnRead();
private:
	CUdpServer m_Server;
	CUdpClient m_Client;
};

class CLiveServer
{
public:
	/**
	 * @note Without / at the end.
	 */
	bool Run(const string&);
	string GetSdp() {return m_Sdp;}
	bool GetTrackID(vector<size_t>&);
	size_t GetPort(size_t);
	void Stop();
private:
	bool Parse(const string&);
private:
	CEventEngin m_Engin;
	string m_Sdp;
	map<size_t, size_t> m_TrackID; // track id, port
	vector<CLiveTrack*> m_Tracks;
};


#endif
