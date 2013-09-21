#ifndef LIVESERVER_H
#define LIVESERVER_H


#include <map>
#include <list>
#include "Event.h"
#include "Socket.h"


using std::map;
using std::list;

class CChannelTrack: public CEventImplement
{
public:
	CChannelTrack(CEventEngin*);
	bool Run(size_t);
	bool Stop();
private:
	int GetFd() {return m_Server.GetFd();}
	void OnRead();
private:
	CUdpServer m_Server;
	CUdpClient m_Client;
};

class CLiveChannel
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
	vector<CChannelTrack*> m_Tracks;
};

class CLiveChannels: public CLogger
{
public:
	static void SetPath(const string &path) {m_Path = path;}
	static void SetIP(const string &ip) {m_IP = ip;}
	static CLiveChannels* GetInstance();
	bool Initialize();
	string GetSdp(const string&);
	bool GetTrackID(const string&, vector<size_t>&);
	size_t GetPort(const string&, size_t);
	void Uninitialize();
private:
	CLiveChannels();
	~CLiveChannels();
private:
	static string m_Path; // Sdp file saved.
	static string m_IP; // Local IP to receive rtp data.
	map<string, CLiveChannel*> m_Channels;
};


#endif
