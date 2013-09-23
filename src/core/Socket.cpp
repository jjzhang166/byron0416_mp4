#include <fcntl.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <errno.h>
#include <string.h>
#include "Socket.h"


/** CSocket */

bool CSocket::SetNonBlock()
{
	int flags = fcntl(m_Fd, F_GETFL, 0);

	if(0 == fcntl(m_Fd, F_SETFL, flags|O_NONBLOCK))
	{
		return true;
	}
	else
	{
		LOG_ERROR("Set NONBLOCK for socket " << m_Fd << " failed(" << strerror(errno) << ").");

		return false;
	}
}

ssize_t CSocket::Send(const void *buf, size_t len)
{
	return send(m_Fd, buf, len, 0);
}

ssize_t CSocket::Recv(void *buf, size_t len)
{
	return recv(m_Fd, buf, len, 0);
}


/** CTcp */

int CTcp::Attach(int fd)
{
	if(m_Fd <= 0)
	{
		if(fd > 0)
			m_Fd = fd;
		else
		{
			m_Fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if(m_Fd == -1)
			{
				LOG_ERROR("Create tcp socket failed(" << strerror(errno) << ").");

				return -1;
			}
		}

		if(true == SetNonBlock())
			return m_Fd;
		else
		{
			if(fd <= 0)
				Close();

			return -1;
		}
	}
	else
	{
		LOG_ERROR("CTcp already have a valid fd " << m_Fd << ".");
		assert(false);
		return -1;
	}
}

bool CTcp::SetLinger(int time)
{
	struct linger lin;

	lin.l_onoff = 1;
	lin.l_linger = time;
	if(0 == setsockopt(m_Fd, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(linger)))
	{
		return true;
	}
	else
	{
		LOG_ERROR("Set LINGER for socket " << m_Fd << " failed(" << strerror(errno) << ").");

		return false;
	}
}

bool CTcp::SetNoDelay()
{
	int nodelay = 1;   

	//if(0 == setsockopt(m_Fd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(char)))   
	if(0 == setsockopt(m_Fd, IPPROTO_IP, TCP_NODELAY, (char*)&nodelay, sizeof(char)))   
		return true;
	else
	{
		LOG_ERROR("Set NODELAY for socket " << m_Fd << " failed(" << strerror(errno) << ").");

		return false;
	}
}


/** CTcpServer */

bool CTcpServer::Listen(int port, const string &ip)
{
	if(-1 == Attach())
		return false;

	struct sockaddr_in addr = {0};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(ip.empty())
	{
		addr.sin_addr.s_addr = htons(INADDR_ANY);
	}
	else
		addr.sin_addr.s_addr = inet_addr(ip.c_str());

	int reuse = 1;
	if(-1 == setsockopt(m_Fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)))
	{
		LOG_ERROR("Call setsockopt(SO_REUSEADDR) failed(" << strerror(errno) << ").");
		Close();

		return false;
	}

	if(bind(m_Fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
	{
		LOG_ERROR("Failed to bind " << ip << ":" << port << "(" << strerror(errno) << ").");
		Close();

		return false;
	}

	if(listen(m_Fd, 65535) == -1)
	{
		LOG_ERROR("Failed to listen with 65535(" << strerror(errno) << ").");
		Close();

		return false;
	}

	return true;
}

int CTcpServer::Accept()
{
	struct sockaddr_in addr = {0};
	socklen_t len = sizeof(sockaddr);

	return accept(m_Fd, (sockaddr*)&addr, &len);
}


/** CTcpClient */

bool CTcpClient::Connect(const string &ip, int port)
{
	if(-1 == Attach())
		return false;

	struct sockaddr_in addr = {0};

	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	addr.sin_port        = htons(port);

	if(connect(m_Fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
	{
		if(errno != EINPROGRESS)
		{
			LOG_ERROR("Failed to connect " << ip << ":" << port << "(" << strerror(errno) << ").");
 
			return false;
		}
	}
 
	return true;
}


/** CUdp */

int CUdp::Attach(int fd)
{
	if(m_Fd <= 0)
	{
		if(fd > 0)
			m_Fd = fd;
		else
		{
			m_Fd = socket(AF_INET, SOCK_DGRAM, 0);
			if(m_Fd == -1)
			{
				LOG_ERROR("Create udp socket failed(" << strerror(errno) << ").");

				return -1;
			}
		}

		if(true == SetNonBlock())
			return m_Fd;
		else
		{
			if(fd <= 0)
				Close();

			return -1;
		}
	}
	else
	{
		LOG_ERROR("CUdp already have a valid fd " << m_Fd << ".");
		assert(false);
		return -1;
	}
}


/** CUdpServer */

bool CUdpServer::Bind(int port, const string &ip)
{
	if(-1 == Attach())
		return false;

	struct sockaddr_in addr = {0};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(ip.empty())
	{
		addr.sin_addr.s_addr = htons(INADDR_ANY);
	}
	else
		addr.sin_addr.s_addr = inet_addr(ip.c_str());

	int reuse = 1;
	if(-1 == setsockopt(m_Fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)))
	{
		LOG_ERROR("Call setsockopt(SO_REUSEADDR) failed(" << strerror(errno) << ").");
		Close();

		return false;
	}

	if(bind(m_Fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
	{
		LOG_ERROR("Failed to bind " << ip << ":" << port << "(" << strerror(errno) << ").");
		Close();

		return false;
	}

	if(ntohl(addr.sin_addr.s_addr) > 0xe0000000)
	{
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(ip.c_str());
		mreq.imr_interface.s_addr = INADDR_ANY;
		if(-1 == setsockopt(m_Fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&mreq, sizeof(mreq)))
		{
			LOG_ERROR("Call setsockopt(IP_ADD_MEMBERSHIP) failed(" << strerror(errno) << ").");
			Close();
			return false;
		}
	}

	return true;
}


/** CUdpClient */

bool CUdpClient::Connect(const string &ip, int port)
{
	if(-1 == Attach())
		return false;

	struct sockaddr_in addr = {0};

	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	addr.sin_port        = htons(port);

	if(connect(m_Fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
	{
		if(errno != EINPROGRESS)
		{
			LOG_ERROR("Failed to connect " << ip << ":" << port << "(" << strerror(errno) << ").");
 
			return false;
		}
	}
 
	return true;
}
