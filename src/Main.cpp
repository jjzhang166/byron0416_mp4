#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include "Disk.h"
#include "LiveServer.h"
#include "Daemon.h"


using namespace std;

void Help()
{
}

int main(int argc, char** argv)
{
	CDaemon keeper;
	string path = argv[0];
	struct stat st;
	bool daemon = true;
	char ch;

	while((ch = getopt(argc, argv, "dh")) != -1)
		switch(ch)
		{
		case 'd':
			{
				daemon = false;
				break;
			}
		case 'h':
			{
				Help();
				exit(0);
			}
		default:
			{
				Help();
				exit(0);
			}
		}

	/** Make log directory. */
	path = path.substr(0, path.rfind("/"));
	string logPath = path + "/log/";
	if(stat(logPath.c_str(), &st) == 0)
	{
		if(!S_ISDIR(st.st_mode))
		{
			unlink(logPath.c_str());
			mkdir(logPath.c_str(), S_IRWXU);
		}
	}
	else
		mkdir(logPath.c_str(), S_IRWXU);

	fstream config((path+"/config").c_str(), ios::in);
	while(config)
	{
		const size_t LEN = 256;
		char buffer[LEN] = {0};

		config.getline(buffer, LEN);
		string param(buffer);
		if(param[0] == '#')
			continue;
		if(param.find("media_dir=") != string::npos)
		{
			string path = param.substr(param.find("=")+1, string::npos);
			if(path[path.size()-1] != '/')
				path += "/";
			CDiskIO::AddPath(path);
		}
		else if(param.find("live_dir=") != string::npos)
		{
			string path = param.substr(param.find("=")+1, string::npos);
			if(path[path.size()-1] != '/')
				path += "/";
			CLiveChannels::SetPath(path);
		}
		else if(param.find("live_ip=") != string::npos)
		{
			string ip = param.substr(param.find("=")+1, string::npos);
			CLiveChannels::SetIP(ip);
		}
		else if(param.find("log_level=") != string::npos)
		{
			istringstream stream(param.substr(param.find("=")+1, string::npos));
			int level = 0;

			stream >> level;
			CLog::SetRootLevel(level*10000);
		}
	}

	return keeper.Run(logPath, daemon);
}
