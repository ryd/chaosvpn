#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "string.h"

bool
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
        memset(buf+s->_u._s.size, 0, growby);
        if (!buf) return false;
        s->_u._s.size += growby;
        s->s = buf;
    }
    memcpy(s->s + s->_u._s.length, sta, len);
    s->_u._s.length += len;
    return true;
}
