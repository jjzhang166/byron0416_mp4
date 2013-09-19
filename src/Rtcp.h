#ifndef RTCP_H
#define RTCP_H


#include "Error.h"


class CRtcp
{
public:
	CRtcp();
	~CRtcp();
	/**
	 * @param len Return used size.
	 */
	ErrorCode Parse(const char*, size_t &len);
private:
	size_t m_Len;
};


#endif
