#ifndef IO_H
#define IO_H


#include "Event.h"


class CBuffer: public CLogger
{
public:
	CBuffer();
	~CBuffer();
	bool Alloc(size_t len);
	void SetOffset(size_t offset) {m_Offset = offset;}
	void SetUsed(size_t used) {m_Used = used;}
	void* GetData(size_t &len);
private:
	const size_t m_Align;
	void *m_Buf;
	size_t m_Len;
	size_t m_Offset;
	size_t m_Used;
};

class CAsyncIO: public CEventImplement
{
public:
	CAsyncIO(CEventEngin *engin): CEventImplement(engin, NULL) {}
	virtual ~CAsyncIO() = 0;
	virtual void AsyncRead(const string &url, size_t offset, size_t len, CEvent *event, CBuffer *buf) = 0;
};


#endif
