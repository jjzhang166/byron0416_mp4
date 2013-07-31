#include "Log.h"
#include "Mp4.h"


int main(int argc, char** argv)
{
	const string path = "3.mp4";
	CMp4Demuxer mp4;

	CLog::SetRootLevel(ALL_LOG_LEVEL);

	mp4.Parse(path);

	return 0;
}
