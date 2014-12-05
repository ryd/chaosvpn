#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "string.h"

bool
string_read(struct string* s, const int fd, const size_t len, intptr_t* bytes_read)
{
    uintptr_t growby;
    char* buf;

    if (len > (s->size - s->length)) {
        growby = s->growby;
        while ((s->size + growby - s->length) < len) {
            growby += s->growby;
            if ((s->size + growby) < s->size) return false;
        }
        buf = realloc(s->s, s->size + growby);
        if (!buf) return false;
        memset(buf+s->size, 0, growby);
        s->size += growby;
        s->s = buf;
    }

    *bytes_read = read(fd, s->s + s->length, len);

    if (*bytes_read > 0)
        s->length += *bytes_read;
    return true;
}
