#include "Log.h"
#include "Mp4Player.h"


int main(int argc, char** argv)
{
	CLog::SetRootLevel(ALL_LOG_LEVEL);

	CUdp udp;
	udp.Attach();
	udp.Connect("192.168.0.101", 1234);

	const string path = "b.mp4";
	CMp4Player rtp(NULL, NULL);

	rtp.Setup(path);
	rtp.Play(udp.GetFd());

	return 0;
}
