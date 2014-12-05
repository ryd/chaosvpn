#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "string.h"

bool
string_putc(struct string* s, char c)
{
    uintptr_t growby;
    char* buf;

    if (s->size == s->length) {
        growby = s->growby;
        buf = realloc(s->s, s->size + growby);
        if (!buf) return false;
        memset(buf+s->size, 0, growby);
        s->size += growby;
        s->s = buf;
    }
    *(s->s + s->length) = c;
    ++s->length;
    return true;
}
