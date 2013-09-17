#ifndef ERROR_H
#define ERROR_H


#include <cstddef>


typedef size_t ErrorCode;

#define E_CONTINUE         100
#define E_OK               200
#define E_BADREQUEST       400
#define E_FORBIDDEN        403
#define E_NOTFOUND         404
#define E_METHODNOTALLOWED 405
#define E_ENTITYTOOLARGE   413
#define E_ERROR            500


#endif
