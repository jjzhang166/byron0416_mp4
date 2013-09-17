#ifndef RTCP_H
#define RTCP_H


#include "Error.h"


class CRtcp
{
public:
	CRtcp();
	~CRtcp();
	ErrorCode Parse(const char*, ssize_t&);
private:
	size_t m_Len;
};


#endif
