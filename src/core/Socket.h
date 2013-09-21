#ifndef SOCKET_H
#define SOCKET_H


#include <unistd.h>
#include "Log.h"


class CSocket: public CLogger
{
public:
	CSocket(): m_Fd(-1) {}
	virtual ~CSocket() {Close();}
	virtual int Attach(int fd) = 0;
	virtual void Detach() {m_Fd = -1;}
	int GetFd() {return m_Fd;}
	bool SetNonBlock();
	ssize_t Send(const void *buf, size_t len);
	ssize_t Recv(void *buf, size_t len);
	void Close()
	{
		if(m_Fd > 0)
		{
			close(m_Fd);
			m_Fd = -1;
		}
	}
protected:
	int m_Fd;
};

class CTcp: public CSocket
{
public:
	int Attach(int fd = -1);
	bool SetLinger(int);
};

class CTcpServer: public CTcp
{
public:
	bool Listen(int port, const string &ip="");
	int Accept();
};

class CTcpClient: public CTcp
{
public:
	bool Connect(const string &ip, int port);
};

class CUdp: public CSocket
{
public:
	int Attach(int fd = -1);
};

class CUdpServer: public CUdp
{
public:
	bool Bind(int port, const string &ip="");
};

class CUdpClient: public CUdp
{
public:
	bool Connect(const string &ip, int port);
};


#endif
