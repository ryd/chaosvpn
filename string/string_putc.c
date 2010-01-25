#include <stdlib.h>
#include <string.h>

#include "string.h"

int
string_putc(struct string* s, char c)
{
    uintptr_t growby;
    char* buf;

    if (s->_u._s.size == s->_u._s.length) {
        growby = s->_u._s.growby;
        buf = malloc(s->_u._s.size + growby);
        if (!buf) return 1;
        s->_u._s.size += growby;
        memcpy(buf, s->s, s->_u._s.length);
        free(s->s);
        s->s = buf;
    }
    *(s->s + s->_u._s.length) = c;
    ++s->_u._s.length;
    return 0;
}
