#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "LiveServer.h"


using namespace std;

/** CChannelTrack */

CChannelTrack::CChannelTrack(CEventEngin *engin):
	CEventImplement(engin, NULL)
{
}

bool CChannelTrack::Run(size_t port)
{
	if(false == m_Client.Connect("239.9.9.9", port))
		return false;

	if(true == m_Server.Bind(port, CLiveChannels::GetIP()))
	{
		if(true == RegisterRD())
			return true;
		else
			return false;
	}
	else
		return false;
}

bool CChannelTrack::Stop()
{
	Unregister();
	m_Server.Close();
	m_Client.Close();

	return true;
}

void CChannelTrack::OnRead()
{
	const size_t LEN = 1500;
	char BUF[LEN] = {0};

	while(true)
	{
		ssize_t len = m_Server.Recv(BUF, LEN);
		if(len > 0)
		{
			//LOG_DEBUG("xxxxxxxxxxxxx" << len);
			len = m_Client.Send(BUF, len);
			if(0 > len)
				LOG_DEBUG("Failed to copy data to multicast " << errno << ".");
		}
		else
			break;
	}
}


/** CLiveChannel */

bool CLiveChannel::Run(const string &path)
{
	if(false == m_Engin.Initialize())
		return false;

	dirent *ent;
	string live = path.substr(path.rfind("/")+1, string::npos);

	/** dark_372_18006_18007.sdp */
	DIR *dir = opendir(path.c_str());
	if(dir != NULL)
	{
		while(NULL != (ent = readdir(dir)))
		{
			string name(ent->d_name);

			if(name.find(".sdp") != string::npos)
			{
				if(name != live+".sdp")
				{
					Parse(path+"/"+name);
					break;
				}
			}
		}
		closedir(dir);
	}
	else
		return false;

	/** dark_372.sdp */
	struct stat st;
	string name = path+"/"+live+".sdp";
	if(0 == stat(name.c_str(), &st))
	{
		ostringstream out;
		fstream sdp(name.c_str(), ios::in);
		out << sdp.rdbuf();
		m_Sdp = out.str();
	}
	else
		return false;

	return true;
}

bool CLiveChannel::GetTrackID(vector<size_t> &ids)
{
	map<size_t, size_t>::iterator iter;

	for(iter=m_TrackID.begin(); iter!=m_TrackID.end(); iter++)
		ids.push_back(iter->first);

	return true;
}

size_t CLiveChannel::GetPort(size_t id)
{
	map<size_t, size_t>::iterator iter = m_TrackID.find(id);

	if(iter != m_TrackID.end())
		return iter->second;
	else
		return 0;
}

void CLiveChannel::Stop()
{
	m_Engin.Stop();

	m_Sdp = "";
	m_TrackID.clear();
	for(size_t i=0; i<m_Tracks.size(); i++)
	{
		m_Tracks[i]->Stop();
		delete m_Tracks[i];
	}
	m_Tracks.clear();

	m_Engin.Uninitialize();
}

bool CLiveChannel::Parse(const string &file)
{
	fstream sdp(file.c_str(), ios::in);
	size_t id = 0;
	size_t port = 0;

	while(sdp)
	{
		const size_t LEN = 256;
		char buffer[LEN+1] = {0};

		sdp.getline(buffer, LEN);
		string line(buffer);

		/** for port */
		size_t pos = line.find("m=");
		if(pos != string::npos)
		{
			pos = line.find(" ");
			line = line.substr(pos+1, string::npos);

			istringstream in(line);
			in >> port;
		}

		/** for id */
		pos = line.find("trackID=");
		if(pos != string::npos)
		{
			line = line.substr(pos+8, string::npos);

			istringstream in(line);
			in >> id;
		}

		if(id!=0 && port!=0)
		{
			CChannelTrack *track = new CChannelTrack(&m_Engin);
			if(true == track->Run(port))
			{
				m_Tracks.push_back(track);
				m_TrackID.insert(pair<size_t, size_t>(id, port));
				id = port = 0;
			}
			else
				return false;
		}
	}

	return true;
}


/** CLiveChannels */

string CLiveChannels::m_Path;
string CLiveChannels::m_IP;

CLiveChannels* CLiveChannels::GetInstance()
{
	static CLiveChannels s_Channels;

	return &s_Channels;
}

bool CLiveChannels::Initialize()
{
	dirent *ent;

	DIR *dir = opendir(m_Path.c_str());
	if(dir != NULL)
	{
		while(NULL != (ent = readdir(dir)))
		{
			if(DT_DIR == ent->d_type)
			{
				CLiveChannel *channel = new CLiveChannel();
				string name = m_Path + ent->d_name;
				if(channel->Run(name) == true)
				{
					m_Channels.insert(pair<string, CLiveChannel*>(string(ent->d_name)+".sdp", channel));
				}
				else
				{
					channel->Stop();
					delete channel;
				}
			}
		}
		closedir(dir);

		return true;
	}
	else
		return false;
}

string CLiveChannels::GetSdp(const string &name)
{
	map<string, CLiveChannel*>::iterator iter = m_Channels.find(name);

	if(iter != m_Channels.end())
		return iter->second->GetSdp();
	else
		return "";
}

bool CLiveChannels::GetTrackID(const string &name, vector<size_t> &ids)
{
	map<string, CLiveChannel*>::iterator iter = m_Channels.find(name);

	if(iter != m_Channels.end())
		return iter->second->GetTrackID(ids);
	else
		return false; 
}

size_t CLiveChannels::GetPort(const string &name, size_t id)
{
	map<string, CLiveChannel*>::iterator iter = m_Channels.find(name);

	if(iter != m_Channels.end())
		return iter->second->GetPort(id);
	else
		return 0; 
}

void CLiveChannels::Uninitialize()
{
	map<string, CLiveChannel*>::iterator iter;
	for(iter=m_Channels.begin(); iter!=m_Channels.end(); iter++)
	{
		iter->second->Stop();
		delete iter->second;
	}
	m_Channels.clear();
}

CLiveChannels::CLiveChannels()
{
}

CLiveChannels::~CLiveChannels()
{
}
