#ifndef LOG_H
#define LOG_H


#include <string>
#include <typeinfo>
#include <log4cplus/loglevel.h>
#include <log4cplus/loggingmacros.h>


using std::string;
using namespace log4cplus;

class CLog
{
public:
	static void SetRootLevel(LogLevel);
	static CLog* GetDefaultLog();
	/**
	 * Initialize a file log.
	 * @param name[in] The name of log4cplus::Logger.
	 * @param dir[in] The directory to save log file.
	 * @param size[in] MB.
	 * @see SetLevel()
	 */
	void Initialize(const string &name, const string &dir, size_t size=100, size_t count=5);
	/**
	 * @see Initialize()
	 */
	void SetLevel(LogLevel);
	Logger GetLog() {return m_Logger;}
private:
	Logger m_Logger;
};

class CLogger
{
public:
	CLogger(): m_Log(CLog::GetDefaultLog()) {}
	virtual void SetLog(CLog *log) {m_Log = log;}
	virtual void SetTitle(const string &title) {m_Title = title;}
protected:
	CLog *m_Log;
	string m_Title;
#define LOG_TRACE(msg) if(m_Log)LOG4CPLUS_TRACE(m_Log->GetLog(), "["  << typeid(*this).name() << "@" << this << "]" << "\t[" << m_Title << "] " << msg);
#define LOG_DEBUG(msg) if(m_Log)LOG4CPLUS_DEBUG(m_Log->GetLog(), "["  << typeid(*this).name() << "@" << this << "]" << "\t[" << m_Title << "] " << msg);
#define LOG_INFO(msg)  if(m_Log)LOG4CPLUS_INFO (m_Log->GetLog(), " [" << typeid(*this).name() << "@" << this << "]" << "\t[" << m_Title << "] " << msg);
#define LOG_WARN(msg)  if(m_Log)LOG4CPLUS_WARN (m_Log->GetLog(), " [" << typeid(*this).name() << "@" << this << "]" << "\t[" << m_Title << "] " << msg);
#define LOG_ERROR(msg) if(m_Log)LOG4CPLUS_ERROR(m_Log->GetLog(), "["  << typeid(*this).name() << "@" << this << "]" << "\t[" << m_Title << "] " << msg);
#define LOG_FATAL(msg) if(m_Log)LOG4CPLUS_FATAL(m_Log->GetLog(), "["  << typeid(*this).name() << "@" << this << "]" << "\t[" << m_Title << "] " << msg);
};


#endif
