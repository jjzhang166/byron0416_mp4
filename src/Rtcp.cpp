#include <arpa/inet.h> //uint16_t
#include "Rtcp.h"


CRtcp::CRtcp():
	m_Len(0)
{
}

CRtcp::~CRtcp()
{
}

ErrorCode CRtcp::Parse(const char *buf, size_t &len)
{
	size_t used = 0;

	while(true)
	{
		if(m_Len > 0)
		{
			size_t min = m_Len<=len ? m_Len : len;
			m_Len -= min;
			used += min;

			len = used;
			if(m_Len == 0)
				return E_OK;
			else
				return E_CONTINUE;
		}
		else if(m_Len == 0)
		{
			m_Len = ntohs(*(uint16_t*)buf);

			len -= 2;
			used += 2;
		}
	}
}
