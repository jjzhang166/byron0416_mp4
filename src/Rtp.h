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

class CRtpSource
{
public:
	virtual ~CRtpSource() = 0;
	virtual size_t GetHintID(vector<size_t>&) = 0;
	virtual string GetSdp(size_t) = 0;
	virtual bool GetRtpSample(size_t, CRtpSample&) = 0;
};

class CRtpPlayer
{
public:
	virtual ~CRtpPlayer() = 0;
	virtual bool Setup(const string&) = 0;
	virtual bool GetTrackID(vector<size_t>&) = 0;
	virtual bool GetSdp(size_t, string&) = 0;
	virtual bool SetInterleaved(size_t, size_t) = 0;
	virtual bool Play(int) = 0;
};


#endif
