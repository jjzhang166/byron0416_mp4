#ifndef DISK_H
#define DISK_H


#include <libaio.h>
#include "IO.h"


class CDiskIO: public CAsyncIO
{
public:
	CDiskIO(CEventEngin*);
	~CDiskIO();
	// CAsyncIO
	void AsyncRead(const string&, size_t, size_t, CEvent*, CBuffer*);
private:
	bool Open(const string &path);
	void Close();
	// CEvent
	int GetFd() {return m_EventFd;}
	void OnRead();
private:
	int m_Fd;
	int m_EventFd;
	io_context_t m_Ctx;
	size_t m_Offset;
	size_t m_Len;
	CBuffer *m_Buf;
};


#endif
