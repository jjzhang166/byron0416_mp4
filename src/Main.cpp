#include "Log.h"
#include "Event.h"
#include "Server.h"
//#include "Mp4Player.h"


int main(int argc, char** argv)
{
	CLog::SetRootLevel(ALL_LOG_LEVEL);

	CServer server;
	server.Run(8554);
	/*
	CUdp udp;
	udp.Attach();
	udp.Connect("192.168.0.101", 1234);

	CEventEngin engin;
	engin.Initialize();

	const string path = "b.mp4";
	CMp4Player rtp(&engin, NULL);

	rtp.Setup(path);
	rtp.Play(udp.GetFd());
	*/

	while(true)
		sleep(1);

	return 0;
}
