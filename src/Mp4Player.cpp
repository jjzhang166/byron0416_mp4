#include <arpa/inet.h> //htonl
#include "Common.h"
#include "Mp4Player.h"


CMp4Player::CMp4Player(CEventEngin *engin, CEvent *pre):
	CEventExImplement(engin, pre)
{
}

bool CMp4Player::Setup(const string &path)
{
	m_StartTime = 0;

	return m_Mp4.Parse(path);
}

bool CMp4Player::GetTrackID(vector<size_t> &ids)
{
	if(0 == m_Mp4.GetHintID(ids))
		return false;
	else
		return true;
}

bool CMp4Player::GetSdp(size_t id, string &sdp)
{
	return m_Mp4.GetSdp(id, sdp);
}

bool CMp4Player::SetInterleaved(size_t id, size_t interleaved)
{
	m_Interleaved.insert(pair<size_t, size_t>(id, interleaved));
}

bool CMp4Player::Play(int fd)
{
	vector<size_t> ids;

	if(m_StartTime == 0)
		m_StartTime = GetCurrent();

	m_Rtsp.Attach(fd);

	if(0 == m_Mp4.GetHintID(ids))
		return false;
	else
	{
		for(size_t i=0; i<ids.size(); i++)
		{
			CRtpSample sample;

			if(true == m_Mp4.GetRtpSample(ids[i], sample))
				m_Samples.insert(std::pair<size_t, CRtpSample>(ids[i], sample));
			break;
		}

		SetTimer(1, 0);
	}

	return true;
}

void CMp4Player::OnTimer()
{
	size_t now = 0;

	if(m_Samples.empty())
	{
		SetTimer(0, 0);
		ReturnSubEvent(E_OK);
	}
	else
		now = GetCurrent();

	map<size_t, CRtpSample>::iterator iter;
	for(iter=m_Samples.begin(); iter!=m_Samples.end(); iter++)
	{
		char interleaved = -1;
		map<size_t, CRtpSample>::iterator i = m_Interleaved.find(iter->first);
		if(i != m_Interleaved)
			interleaved  = i->second;
		else
			continue;

		while(true)
		{
			CRtpSample &sample = iter->second;
			size_t ts = sample.m_Timestamp;
			if(ts < now-m_StartTime)
			{
				vector<CRtpPacket> &packets = sample.m_Packets;

				for(size_t i=0; i<packets.size(); i++)
				{
					char num[2] = {0x24, interleaved};
					UInt16 len = htons(packets[i].m_Len);
					m_Rtsp.Send(num, 2);
					m_Rtsp.Send(&len, 2);
					ssize_t ret = m_Rtsp.Send(packets[i].m_Packet, packets[i].m_Len);
					LOG_TRACE("Send a rtp packet with " << packets[i].m_Len << "/" << ret << " bytes.");
				}
				packets.clear();
				if(false == m_Mp4.GetRtpSample(iter->first, sample))
				{
					m_Samples.erase(iter);
					break;
				}
			}
			else
			{
				ts -= now-m_StartTime;
				SetTimer(ts, 0);
				break;
			}
		}
	}
}
