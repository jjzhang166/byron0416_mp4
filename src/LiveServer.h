#ifndef LIVESERVER_H
#define LIVESERVER_H


#include <map>
#include "Event.h"


using std::map;

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
private:
	bool Parse(const string&);
private:
	string m_Sdp;
	string m_IP;
	map<size_t, size_t> m_Tracks; // track id, port
};


#endif
