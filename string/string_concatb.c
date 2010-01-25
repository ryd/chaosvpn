#include <stdlib.h>
#include <string.h>

#include "string.h"

int
string_concatb(struct string* s, const char* sta, uintptr_t len)
{
    uintptr_t growby;
    char* buf;

    if (len > (s->_u._s.size - s->_u._s.length)) {
        growby = s->_u._s.growby;
        while ((s->_u._s.size + growby - s->_u._s.length) < len) {
            growby += s->_u._s.growby;
            if ((s->_u._s.size + growby) < s->_u._s.size) return 1;
        }
        buf = realloc(s->s, s->_u._s.size + growby);
        if (!buf) return 1;
        s->_u._s.size += growby;
        s->s = buf;
    }
    memcpy(s->s + s->_u._s.length, sta, len);
    s->_u._s.length += len;
    return 0;
}
