#include <string.h>
#include "IO.h"


/** CBuffer */

CBuffer::CBuffer(): m_Align(512), m_Buf(NULL), m_Len(0), m_Offset(0), m_Used(0)
{
}

CBuffer::~CBuffer()
{
	if(m_Buf)
		free(m_Buf);
}

bool CBuffer::Alloc(size_t len)
{
	m_Offset = 0;
	m_Used = 0;

	if(m_Buf)
	{
		if(m_Len >= len)
		{
			memset(m_Buf, 0, m_Len);

			return true;
		}
		else
		{
			free(m_Buf);
			m_Buf = NULL;
			m_Len = 0;
		}
	}

	int ret = posix_memalign(&m_Buf, m_Align, len);
	if(ret!=0 || m_Buf==NULL)
	{
		LOG_FATAL("Failed to alloc memory for " << len << "/" << m_Align << " bytes.");

		return false;
	}
	else
	{
		m_Len = len;
		memset(m_Buf, 0, m_Len);

		return true;
	}
}

void* CBuffer::GetData(size_t &len)
{
	len = m_Used;

	return (char*)m_Buf + m_Offset;
}


/* CAsyncIO */

CAsyncIO::~CAsyncIO()
{
}
