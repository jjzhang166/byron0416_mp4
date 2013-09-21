#include <sys/wait.h>
#include <errno.h>
#include "Server.h"
#include "LiveServer.h"
#include "Daemon.h"


/** CDaemon */

bool CDaemon::m_Run = true;

int CDaemon::Run(const string &log, bool isDaemon)
{
	signal(SIGINT, SignalHandle);
	signal(SIGKILL, SignalHandle);

	pid_t pidWorker = -1;
	pid_t pidExit= -1;
	int status = 0;

	while(m_Run)
	{
		if(pidWorker == -1)
			pidWorker = ForkWorker(log, isDaemon);
		if(pidWorker == -1)
		{
			LOG_FATAL("Cannot fork worker process(" << errno << ").");

			return 1;
		}
		else if(pidWorker == 0) //isDaemon == false
			return 0;
		else
		{
			pidExit = wait(&status);
			if(pidExit > 0)
			{
				if(WIFEXITED(status) != 0)
				{
					LOG_FATAL("Worker process " << pidExit << " quit with error " << WEXITSTATUS(status) << "!");

					return 2;
				}
				if(WIFSIGNALED(status))
				{
					LOG_FATAL("Worker process " << pidExit << " quit with signal " << WTERMSIG(status) << "!");
					if(pidWorker == pidExit)
					{
						pidWorker = -1;
					}
				}
			}
			else
			{
				LOG_FATAL("Failed to wait worker process to exit(" << errno << ").");

				return 3;
			}
		}
	}

	return 0;
}

pid_t CDaemon::ForkWorker(const string &log, bool isDaemon)
{
	pid_t pid = 0;

	if(isDaemon == true)
	{
		daemon(1, 0);
		pid = fork();
	}

	if(pid == 0)
	{
		signal(SIGPIPE, SIG_IGN);
		CServer server;
		CLog fileLog;
		const string logName = "rtsp.log";

		fileLog.Initialize(logName, log);

		m_Log = &fileLog;

		CLiveChannels::GetInstance()->Initialize();

		server.SetLog(&fileLog);
		if(false == server.Run(8080))
		{
			LOG_FATAL("Failed to run rtsp server.");

			exit(1);
		}

		while(m_Run)
			sleep(1);

		CLiveChannels::GetInstance()->Uninitialize();
	}

	return pid;
}

void CDaemon::SignalHandle(int sig)
{
	m_Run = false;
}
