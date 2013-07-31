#include <log4cplus/helpers/pointer.h>
#include <log4cplus/layout.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include "Log.h"


/** CLog */

const string PATTERN = "%D{%x %X,%q} %p %m%n";

void CLog::SetRootLevel(LogLevel level)
{
	Logger::getRoot().setLogLevel(level);
}

CLog* CLog::GetDefaultLog()
{
	static CLog s_Log;
	static volatile bool s_Init = false; 

	if(s_Init == false)
	{
		std::auto_ptr<Layout> layout(new PatternLayout(PATTERN));

		helpers::SharedObjectPtr<Appender> append(new ConsoleAppender());
		append->setLayout(layout);

		s_Log.m_Logger = Logger::getInstance("Default");
		s_Log.m_Logger.addAppender(append);

		s_Init = true;
	}

	return &s_Log;
}

void CLog::Initialize(const string &name, const string &path, size_t size, size_t count)
{
	std::auto_ptr<Layout> layout(new PatternLayout(PATTERN));

	helpers::SharedObjectPtr<Appender> append(new RollingFileAppender(path+name, size<<20, count));
	append->setLayout(layout);

	m_Logger = Logger::getInstance(name);
	m_Logger.addAppender(append);
}

void CLog::SetLevel(LogLevel level)
{
	m_Logger.setLogLevel(level);
}
