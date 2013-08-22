#include "Log.h"
#include "Mp4.h"


int main(int argc, char** argv)
{
	CLog::SetRootLevel(ALL_LOG_LEVEL);

	//const string path = "3.mp4";
	const string path = "b.mp4";
	CMp4Demuxer mp4;

	mp4.Parse(path);
	mp4.Send(path);

	return 0;
}
