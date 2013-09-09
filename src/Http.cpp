#include <sstream>
#include "Http.h"


using std::istringstream;
using std::ostringstream;
using std::pair;

/** CHttpRequest */

void CHttpRequest::Initialize()
{
	m_Request = "";
	m_Method = "";
	m_Url = "";
	m_ParamString = "";
	m_FullUrl = "";
	m_Suffix = "";
	m_Params.clear();
	m_Fields.clear();
}

ErrorCode CHttpRequest::Parse(const string &buf)
{
	m_Request += buf;

	if(m_Request.size() > 2048)
		return E_ENTITYTOOLARGE;

	if(m_Request.find("\r\n\r\n") == string::npos)
		return E_CONTINUE;

	istringstream lines(m_Request);
	const size_t LEN = 1024;
	char tmp[LEN] = {0};

	char step = 1;
	while(lines.getline(tmp, LEN))
	{
		string line(tmp);
		ErrorCode err;

		if(line == "\r") /** The lastest line. */
			break;
		else
			line = line.substr(0, line.find("\r")); /** Remove \r. */

		switch(step)
		{
		case 1: /** URL */
			err = ParseUrl(line);
			if(E_OK != err)
				return err;
			else
			{
				step = 2;
				break;
			}
		case 2: /** Fields */
			if(false == ParseField(line))
				return E_BADREQUEST;
			else
				break;
		};
	}

	return E_OK;
}

string CHttpRequest::GetMime()
{
	if(m_Suffix == "m3u")
		return "audio/mpegurl";
	else if(m_Suffix == "ts")
		return "video/mpeg";
	else
		return "application/octet-stream";
}

bool CHttpRequest::GetParam(const string &name, string &value) const
{
	map<string, string>::const_iterator iter = m_Params.find(name);

	if(iter != m_Params.end())
	{
		value = iter->second;

		return true;
	}
	else 
		return false;
}

bool CHttpRequest::GetField(const string &name, string &value) const
{
	map<string, string>::const_iterator iter = m_Fields.find(name);

	if(iter != m_Fields.end())
	{
		value = iter->second;

		return true;
	}
	else 
		return false;
}

inline ErrorCode CHttpRequest::ParseUrl(const string &url)
{
	/** method */
	m_Method = url.substr(0, url.find(" "));

	/** full url */
	m_FullUrl = url.substr(url.find(" ")+1, string::npos);
	m_FullUrl = m_FullUrl.substr(0, m_FullUrl.find(" "));
	if(m_FullUrl.find("?") != string::npos)
	{
		/** url */
		m_Url = m_FullUrl.substr(0, m_FullUrl.find("?"));

		/** param */
		m_ParamString = m_FullUrl.substr(m_FullUrl.find("?")+1, string::npos);
		if(false == ParseParam(m_ParamString))
			return E_BADREQUEST;
	}
	else
		m_Url = m_FullUrl;

	/** file */
	size_t pos = m_Url.find("://");
	if(pos != string::npos)
	{
		pos = m_Url.find("/", pos+3);
		m_File = m_Url.substr(pos+1, string::npos);
	}
	else
		m_File = m_Url;

	m_Suffix = m_Url.substr(m_Url.rfind(".")+1, string::npos);

	return E_OK;
}

inline bool CHttpRequest::ParseParam(const string &params)
{
	string tmp = params;
	string name;
	string value;
	size_t pos;

	while(!tmp.empty())
	{
		pos = tmp.find("=");
		if(pos != string::npos)
		{
			/** Get name */
			name = tmp.substr(0, pos); 
			tmp = tmp.substr(pos+1, string::npos);

			/** Get value */
			pos = tmp.find("&");
			value = tmp.substr(0, pos);

			/** Save parameters. */
			m_Params.insert(pair<string, string>(name, value));

			/** split tmp */
			if(pos != string::npos)
				tmp = tmp.substr(pos+1, string::npos);
			else
				break;
		}
		else
			return false;
	}

	return true;
}

inline bool CHttpRequest::ParseField(const string &field)
{
	size_t pos = field.find(": ");

	if(pos != string::npos)
	{
		string name = field.substr(0, pos);
		string value = field.substr(pos+2, string::npos);
		m_Fields.insert(pair<string, string>(name, value));

		return true;
	}
	else
		return false;
}


/** CHttpResponse */

void CHttpResponse::Initialize()
{
	m_Header = "";
	m_Fields.clear();
}

void CHttpResponse::AddField(const string &name, string &value)
{
	pair<map<string, string>::iterator, bool> ret = m_Fields.insert(pair<string, string>(name, value));
	if(ret.second == false)
		ret.first->second = value;
}

void CHttpResponse::Response(const string &version, ErrorCode err)
{
	ostringstream out;
	out << version << " " << err << " ";

	switch(err)
	{
	case E_OK:
		out << "OK";
		break;
	case E_BADREQUEST:
		out << "Bad Request";
		break;
	case E_FORBIDDEN:
		out << "Forbidden";
		break;
	case E_NOTFOUND:
		out << "Not Found";
		break;
	case E_METHODNOTALLOWED:
		out << "Method Not Allowed";
		break;
	case E_ENTITYTOOLARGE:
		out << "Request Entity Too Large";
		break;
	case E_ERROR:
		out << "Internal Server Error";
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

	m_Header = out.str();
}
