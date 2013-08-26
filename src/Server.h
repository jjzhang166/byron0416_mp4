#ifndef SERVER_H
#define SERVER_H


#include <list>
#include "Event.h"
#include "Socket.h"
#include "Session.h"


using std::list;

class CEngin: public CEventExImplement
{
public:
	CEngin();
	~CEngin();
	/**
	 * Start the engin.
	 * @param[in] listener File descriptor of socket to listen.
	 * @param[in] affinity Set affinity with cpus.
	 */
	bool Run(int listener, size_t affinity = -1);
	size_t GetFree() {return m_Free;}
	size_t GetDone() {return m_Done;}
	//CLogger
	void SetLog(CLog*);
private:
	//CEventEx
	int GetFd() {return m_Listener.GetFd();}
	void OnRead();
	void OnSubEvent(const CEvent*, ErrorCode);
	void OnTimer();
private:
	static CMutex m_Mutex;
	const size_t m_SessionCount;
	CEventEngin m_Engin;
	CTcpServer m_Listener;
	list<CSession*> m_Sessions;
	volatile size_t m_Free;
	volatile size_t m_Done;
};

class CServer: public CEventExImplement
{
public:
	CServer();
	~CServer();
	/**
	 * Start the server
	 * @param[in] port Server will listen at port.
	 * @param[in] ip Server will listen at all address if ip is empty.
	 */
	bool Run(size_t port, const string &ip="");
	//CLogger
	void SetLog(CLog *log);
private:
	//CEventEx
	int GetFd() {return 0;}
	void OnTimer();
private:
	size_t m_EnginCount;
	CEventEngin m_Engin;
	CTcpServer m_Listener;
	vector<CEngin*> m_Engins;
};


#endif
