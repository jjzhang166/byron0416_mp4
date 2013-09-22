#include <arpa/inet.h> // htons
#include "LiveServer.h"
#include "LivePlayer.h"


/** CLiveTrack */

CLiveTrack::CLiveTrack(CEventEngin *engin):
	CEventImplement(engin, NULL), m_Port(0), m_ID(0), m_Interleaved(0)
{
}

bool CLiveTrack::Run()
{
	if(true == m_Server.Bind(m_Port, "239.9.9.9"))
		return RegisterRD();
	else
		return false;
}

void CLiveTrack::Stop()
{
	Unregister();
	m_Rtsp.Detach();
	m_Server.Close();
}

void CLiveTrack::OnRead()
{
	const size_t LEN = 1500;
	char BUF[LEN+1];

	while(true)
	{
		ssize_t len = m_Server.Recv(BUF, LEN);
		if(len > 0)
		{
			//LOG_DEBUG("Get a udp packet with bytess " << len << " " << size_t(m_Interleaved));
			char num[2] = {0x24, m_Interleaved}; //$
			uint16_t l = htons(len);
			m_Rtsp.Send(num, 2);
			m_Rtsp.Send(&l, 2);
			m_Rtsp.Send(BUF, len);
		}
		else
			break;
	}
}


/** CLivePlayer */

CLivePlayer::CLivePlayer(CEventEngin *engin, CEvent *pre):
	CEventImplement(engin, pre)
{
	CLiveTrack track(m_Engin);

	m_Tracks.push_back(track);
	m_Tracks.push_back(track);
}

bool CLivePlayer::Setup(const string &name)
{
	m_Sdp = CLiveChannels::GetInstance()->GetSdp(name);
	if(false == m_Sdp.empty())
	{
		vector<size_t> ids;

		m_Name = name;
		CLiveChannels::GetInstance()->GetTrackID(name, ids);
		for(size_t i=0; i<m_Tracks.size(); i++)
		{
			if(i < ids.size())
			{
				m_Tracks[i].SetID(ids[i]);
				size_t port = CLiveChannels::GetInstance()->GetPort(name, ids[i]);
				m_Tracks[i].SetPort(port);
			}
			else
				break;
		}

		return true;
	}
	else
		return false;
}

bool CLivePlayer::GetTrackID(vector<size_t> &ids)
{
	return CLiveChannels::GetInstance()->GetTrackID(m_Name, ids);
}

string CLivePlayer::GetSdp()
{
	return m_Sdp;
}

bool CLivePlayer::SetInterleaved(size_t id, size_t interleaved)
{
	for(size_t i=0; i<m_Tracks.size(); i++)
	{
		if(m_Tracks[i].GetID() == id)
		{
			m_Tracks[i].SetInterleaved(interleaved);

			return true;
		}
	}

	return false;
}

bool CLivePlayer::Play(int fd)
{
	for(size_t i=0; i<m_Tracks.size(); i++)
	{
		if(false == m_Tracks[i].SetFd(fd))
			return false;
		if(false == m_Tracks[i].Run())
			return false;
	}

	return true;
}

bool CLivePlayer::Pause()
{
	return false;
}

bool CLivePlayer::Resume()
{
	return false;
}

void CLivePlayer::Teardown()
{
	for(size_t i=0; i<m_Tracks.size(); i++)
	{
		m_Tracks[i].Stop();
	}
}

void CLivePlayer::SetLog(CLog *log)
{
	m_Log = log;

	for(size_t i=0; i<m_Tracks.size(); i++)
		m_Tracks[i].SetLog(m_Log);
}

void CLivePlayer::SetTitle(const string &title)
{
	m_Title = title;

	for(size_t i=0; i<m_Tracks.size(); i++)
		m_Tracks[i].SetTitle(m_Title);
}
