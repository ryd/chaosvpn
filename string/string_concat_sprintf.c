#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "string.h"

int string_concat_sprintf(struct string* s, const char *msg, ...)
{
    va_list args;
    char* buffer;

    buffer = malloc(4096);
    if (!buffer) return 1;
    
    va_start(args, msg);
    // TODO: reimplement vsnprintf to directly use string_concatb
    vsnprintf(buffer, 4096, msg, args);
    va_end(args);
    
    if (string_concatb(s, buffer, strlen(buffer))) {
        goto bail_out;
        return 1;
    }
    if (string_concatb(s, "\0", 1)) {
        goto bail_out;
        return 1;
    }
    s->_u._s.length--;
    free(buffer);
    return 0;

bail_out:
    free(buffer);
    return 1;
}
