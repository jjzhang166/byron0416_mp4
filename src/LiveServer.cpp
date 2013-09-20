#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include "LiveServer.h"


using namespace std;

bool CLiveServer::Run(const string &path)
{
	dirent *ent;

	/** dark_372_18006_18007.sdp */
	DIR *dir = opendir(path.c_str());
	if(dir != NULL)
	{
		while(NULL != (ent = readdir(dir)))
		{
			string name(ent->d_name);

			if(name.find(".sdp") != string::npos)
			{
				if(name != path+".sdp")
				{
					Parse(path+"/"+name);
					break;
				}
			}
		}
		closedir(dir);
	}
		return false;

	/** dark_372.sdp */
	struct stat st;
	string name = path+"/"+path+".sdp";
	if(0 == stat(name.c_str(), &st))
	{
		ostringstream out;
		fstream sdp(name.c_str(), ios::in);
		out << sdp.rdbuf();
		m_Sdp = out.str();
	}

	return true;
}

bool CLiveServer::GetTrackID(vector<size_t> &ids)
{
	map<size_t, size_t>::iterator iter;

	for(iter=m_Tracks.begin(); iter!=m_Tracks.end(); iter++)
		ids.push_back(iter->first);

	return true;
}

size_t CLiveServer::GetPort(size_t id)
{
	map<size_t, size_t>::iterator iter = m_Tracks.find(id);

	if(iter != m_Tracks.end())
		return iter->second;
	else
		return 0;
}

bool CLiveServer::Parse(const string &file)
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
			m_Tracks.insert(pair<size_t, size_t>(id, port));
			id = port = 0;
		}
	}

	return true;
}
