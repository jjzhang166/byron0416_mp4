#ifndef RTP_H
#define RTP_H


#include <string>
#include <vector>


using std::string;
using std::vector;

#define RTP_MTU 1460

class CRtpPacket
{
public:
	size_t m_Len;
	char m_Packet[RTP_MTU];
};

class CRtpSample
{
public:
	size_t m_Timestamp;
	vector<CRtpPacket> m_Packets;
};

class CRtpPlayer
{
public:
	virtual ~CRtpPlayer() = 0;
public:
	virtual bool Setup(const string&) = 0;
	virtual bool SetInterval(const string&, int) = 0;
	virtual bool Play(int) = 0;
};


#endif
