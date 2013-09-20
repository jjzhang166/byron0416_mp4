#include "Log.h"
#include "Event.h"
#include "Disk.h"
#include "Server.h"


int main(int argc, char** argv)
{
	CLog::SetRootLevel(ALL_LOG_LEVEL);
	//CLog::SetRootLevel(INFO_LOG_LEVEL);

	CServer server;
	server.Run(8080);

	while(true)
		sleep(1);

	return 0;
}
