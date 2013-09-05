#ifndef RTSP_H
#define RTSP_H


#include "Http.h"


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
