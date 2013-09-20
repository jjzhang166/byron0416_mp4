#ifndef LIVEPLAYER_H
#define LIVEPLAYER_H


class CLivePlayer: public CRtpPlayer, public CEventExImplement
{
	/** CRtpPlayer */
	bool Setup(const string&);
	bool GetTrackID(vector<size_t>&);
	bool GetSdp(size_t, string&);
	bool SetInterleaved(size_t, size_t);
	bool Play(int);
	void Teardown();
private:
	/** CEventExImplement */
	int GetFd() {return -1;}
	void OnTimer();
private:
	map<size_t, size_t> m_Interleaved;
};

#endif
