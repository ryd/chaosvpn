#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "string.h"

int string_concat_sprintf(struct string* s, const char *msg, ...)
{
    va_list args;
    char buffer[4096];
    
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    
    if (string_concatb(s, buffer, strlen(buffer))) return 1;
    if (string_concatb(s, "\0", 1)) return 1;
    s->_u._s.length--;
    return 0;
}
