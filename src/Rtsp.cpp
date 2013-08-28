#include <sstream>
#include "Rtsp.h"


using namespace std;

/* CRtspRequest */

size_t CRtspRequest::Parse(const string &buf)
{
	if(buf.find("\r\n\r\n") == string::npos)
		return 0;
	else
		m_Fields.clear();

	if(buf.size() > 2048)
		return 413;

	LOG_INFO("Get a rtsp request:\n" << buf);

	stringstream lines(buf);
	const size_t LEN = 1024;
	char tmp[LEN] = {0};

	char step = 1;
	while(lines.getline(tmp, LEN))
	{
		string line(tmp);
		size_t code;

		if(line == "\r")
			break;

		switch(step)
		{
			case 1:
				code = ParseUrl(line);
				if(200 != code)
					return code;
				else
				{
					step = 2;
					break;
				}
			case 2:
				if(false == ParseField(line))
					return 400;
				else
					break;
		};
	}

	return 200;
}

bool CRtspRequest::GetParam(const string &name, string &value)
{
	map<string, string>::iterator iter = m_Params.find(name);

	if(iter != m_Fields.end())
	{
		value = iter->second;

		return true;
	}
	else 
		return false;
}

bool CRtspRequest::GetField(const string &name, string &value)
{
	map<string, string>::iterator iter = m_Fields.find(name);

	if(iter != m_Fields.end())
	{
		value = iter->second;

		return true;
	}
	else 
		return false;
}

size_t CRtspRequest::ParseUrl(const string &url)
{
	m_Method = url.substr(0, url.find(" "));

	//if(m_Method == "GET")
	{
		if(url.substr(url.rfind(" ")+1, string::npos) != "RTSP/1.0\r")
			m_FullUrl = url.substr(url.find(" ")+1, string::npos);
		m_FullUrl = m_FullUrl.substr(0, m_FullUrl.find(" "));
		if(m_FullUrl.find("?") != string::npos)
		{
			return 400;
		}
		else
			return 200;

	}
	/*
	else
	{
		LOG_ERROR("Unsupport method(" << m_Method << ") for rtsp.")

			return 405;
	}
	*/
}

bool CRtspRequest::ParseParam(string paramList)
{
	size_t pos;
	paramList += "&";

	while(string::npos != (pos = paramList.find("&")))
	{
		string param = paramList.substr(0, pos);
		size_t eq = param.find("=");
		if(eq == string::npos)
			return false;
		else
		{
			string key = param.substr(0, eq);
			if(key.empty())
				return false;
			m_Params.insert(pair<string, string>(key, param.substr(eq+1, string::npos)));
			paramList = paramList.substr(pos+1, string::npos);
		}
	}

	return true;
}

bool CRtspRequest::ParseField(const string &field)
{
	size_t pos = field.find(": ");
	string key = field.substr(0, pos);
	string value = field.substr(pos+2, string::npos);
	m_Fields.insert(pair<string, string>(key, value.substr(0, value.find("\r"))));

	return true;
}


/* CRtspResponse */


void CRtspResponse::AddField(const string &name, string &value)
{
	m_Fields.insert(pair<string, string>(name, value));
}

string CRtspResponse::Response(size_t code)
{
	ostringstream out;

	out << "RTSP/1.0 " << code << " ";
	switch(code)
	{
	case 200:
		out << "OK";
		break;
	case 400:
		out << "Bad Request";
		break;
	case 403:
		out << "Forbidden";
		break;
	case 404:
		out << "Not Found";
		break;
	case 405:
		out << "Method Not Allowed";
		break;
	case 413:
		out << "Request Entity Too Large";
		break;
	case 500:
		out << "Internal Server Error";
		break;
	case 505:
		out << "Version not supported";
		break;
	default:
		out << "Internal Server Error";
	}
	out << "\r\n";

	map<string, string>::iterator iter;
	for(iter=m_Fields.begin(); iter!=m_Fields.end(); iter++)
	{
		out << iter->first << ": " << iter->second << "\r\n";
	}

	out << "\r\n";

	//LOG_INFO("Send a http response:\n" << out.str());
	m_Fields.clear();

	return out.str();
}
