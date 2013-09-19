#ifndef HTTP_H
#define HTTP_H


#include <map>
#include "Error.h"
#include "Log.h"


using std::map;

class CHttpRequest: public CLogger
{
public:
	/**
	 * Initialize all members.
	 */
	void Initialize();
	/**
	 * Parse http request.
	 * @param len retrun used length.
	 * @retval E_CONTINUE Have not got whole entity.
	 * @retval E_OK Have parsed all http request.
	 * @retval E_BADREQUEST
	 * @retval E_METHODNOTALLOWED
	 * @retval E_ENTITYTOOLARGE
	 */
	ErrorCode Parse(const char*, ssize_t &len);
	string GetRequest() const {return m_Request;}
	string GetMethod() const {return m_Method;}
	string GetFullUrl() const {return m_FullUrl;}
	/**
	 * Get url without parameters from a http request.
	 */
	string GetUrl() const {return m_Url;}
	string GetParams() const {return m_ParamString;}
	string GetFile() const {return m_File;}
	string GetMime();
	bool GetParam(const string &name, string &value) const;
	bool GetField(const string &name, string &value) const;
private:
	ErrorCode ParseUrl(const string &url);
	bool ParseParam(const string &params);
	bool ParseField(const string &field);
private:
	string m_Request;
	string m_Method;
	string m_FullUrl;
	string m_Url;
	string m_ParamString;
	string m_File;
	string m_Suffix;
	map<string, string> m_Params;
	map<string, string> m_Fields;
};

class CHttpResponse: public CLogger
{
public:
	/**
	 * Initialize all members.
	 */
	void Initialize();
	void AddField(const string &name, string &value);
	/**
	 * Set a http response header when got a error.
	 */
	void Response(const string&, ErrorCode);
	string GetHeader() const {return m_Header;}
private:
	string m_Header;
	map<string, string> m_Fields;
};


#endif
