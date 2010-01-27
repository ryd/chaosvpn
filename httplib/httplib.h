#ifndef __HTTPLIB_H_
#define __HTTPLIB_H_

#include "../string/string.h"

extern int http_parseurl(struct string*, struct string*, int*, struct string*);
int http_get(struct string*, struct string*, time_t, int*, struct string*);

#endif
