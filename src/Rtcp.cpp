#include <arpa/inet.h> //uint16_t
#include "Rtcp.h"


CRtcp::CRtcp():
	m_Len(0)
{
}

CRtcp::~CRtcp()
{
}

ErrorCode CRtcp::Parse(const char *buf, ssize_t &len)
{
	while(true)
	{
		if(m_Len > 0)
		{
			size_t min = m_Len<=len ? m_Len : len;
			m_Len -= min;
			len -= min;

			if(m_Len == 0)
				return E_OK;
			else
				return E_CONTINUE;
		}
		else if(m_Len == 0)
		{
			m_Len = *(uint16_t*)buf;
			len -= 2;
		}
	}
}
