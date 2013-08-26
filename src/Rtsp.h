#ifndef RTSP_H
#define RTSP_H


#include <map>
#include <string>
#include <Log.h>


using std::map;
using std::string;

class CRtspRequest: public CLogger
{
public:
	size_t Parse(const string &buf);
	string GetUrl() const {return m_Url;}
	string GetMethod() {return m_Method;}
	bool GetParam(const string &name, string &value);
	bool GetField(const string &name, string &value);
private:
	size_t ParseUrl(const string &url);
	bool ParseParam(string paramList);
	bool ParseField(const string &field);
private:
	string m_Method;
	string m_FullUrl;
	string m_Url;
	map<string, string> m_Params;
	map<string, string> m_Fields;
};

class CRtspResponse: public CLogger
{
public:
	void SetCode(size_t code) {m_Code = code;}
	void AddField(const string &name, string &value);
	string Response(size_t code);
private:
	size_t m_Code;
	map<string, string> m_Fields;
};


#endif
