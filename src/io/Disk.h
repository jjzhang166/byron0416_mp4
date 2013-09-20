#ifndef DISK_H
#define DISK_H


#include <vector>
#include <libaio.h>
#include "IO.h"


using std::vector;

class CDiskIO: public CAsyncIO
{
public:
	static void AddPath(const string&);
	CDiskIO(CEventEngin*);
	~CDiskIO();
	// CAsyncIO
	void AsyncRead(const string&, size_t, size_t, CEvent*, CBuffer*);
private:
	size_t GetFilePath(string&);
	/**
	 * @return File size, 0 if have not found.
	 * @note Cannot open empty file.
	 */
	size_t Open(const string&);
	void Close();
	// CEvent
	int GetFd() {return m_EventFd;}
	void OnRead();
private:
	static vector<string> m_Paths;
	int m_Fd;
	size_t m_Offset;
	size_t m_Len;
	CBuffer *m_Buf;
	int m_EventFd;
	io_context_t m_Ctx;
};


#endif
