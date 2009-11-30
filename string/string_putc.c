#include <stdlib.h>
#include <string.h>

#include "string.h"

int string_putc(struct string* s, char c)
{
    int growby;
    char* buf;
    int len = 1;

    if (len > (s->_u._s.size - s->_u._s.length)) {
        growby = s->_u._s.growby;
        while ((s->_u._s.size + growby - s->_u._s.length) < len) {
            growby += s->_u._s.growby;
            if ((s->_u._s.size + growby) < s->_u._s.size) return 1;
        }
        buf = malloc(s->_u._s.size + growby);
        if (!buf) return 1;
        s->_u._s.size += growby;
        memcpy(buf, s->s, s->_u._s.length);
        free(s->s);
        s->s = buf;
    }
    *(s->s + s->_u._s.length) = c;
    s->_u._s.length += len;
    return 0;
}
