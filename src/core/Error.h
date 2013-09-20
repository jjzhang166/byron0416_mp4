#ifndef ERROR_H
#define ERROR_H


enum ErrorCode
{
	E_CONTINUE         = 100,
	E_OK               = 200,
	E_BADREQUEST       = 400,
	E_FORBIDDEN        = 403,
	E_NOTFOUND         = 404,
	E_METHODNOTALLOWED = 405,
	E_ENTITYTOOLARGE   = 413,
	E_ERROR            = 500
};


#endif
