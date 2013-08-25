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

bool CMp4Player::Play(int fd)
{
	vector<size_t> ids;

	if(m_StartTime == 0)
		m_StartTime = GetCurrent();

	if(0 == m_Mp4.GetHintID(ids))
		return false;
	else
	{
		for(size_t i=0; i<ids.size(); i++)
		{
			CRtpSample sample;

			if(true == m_Mp4.GetRtpSample(ids[i], sample))
				m_Samples.insert(pair<size_t, CRtpSample>(ids[i], sample));
		}
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
		while(true)
		{
			CRtpSample &sample = iter->second;
			size_t ts = sample.m_Timestamp;
			if(ts < now-m_StartTime)
			{
				vector<CRtpPacket> &packets = sample.m_Packets;

				for(size_t i=0; i<packets.size(); i++)
					m_Tcp.Send(packets[i].m_Packet, packets[i].m_Len);
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
