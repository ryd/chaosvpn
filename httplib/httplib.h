#ifndef __HTTPLIB_H
#define __HTTPLIB_H

#include "../string/string.h"

static const int HTTP_EOK = 0;
static const int HTTP_EINVURL = 1;
static const int HTTP_ENOMEM = 2;
static const int HTTP_ENETERR = 3;
static const int HTTP_ESRVERR = 4;

extern int http_parseurl(struct string*, struct string*, int*, struct string*);
int http_get(struct string*, struct string*, time_t, struct string*, int*, struct string*);

#endif
