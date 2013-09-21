#ifndef DAEMON_H
#define DAEMON_H


#include "Log.h"


class CDaemon: public CLogger
{
public:
	/**
	 * @param[in] log The path to save log file.
	 */
	int Run(const string &log, bool isDaemon = true);
private:
	pid_t ForkWorker(const string&, bool);
	static void SignalHandle(int sig);
private:
	static bool m_Run;
};


#endif
