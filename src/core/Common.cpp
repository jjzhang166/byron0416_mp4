#include <sys/time.h>
#include "Common.h"


size_t GetCurrent()
{
	struct timeval tv;
	struct timezone tz;

	if(gettimeofday(&tv, &tz) == 0)
	{
		return (size_t)tv.tv_sec*1000 + tv.tv_usec/1000;
	}
	else
	{
		assert(false);
		return 0;
	}
}
