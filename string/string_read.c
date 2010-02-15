#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "string.h"

int
string_read(struct string* s, int fd, uintptr_t len, intptr_t* bytes_read)
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
    *bytes_read = read(fd, s->s + s->_u._s.length, len);
    s->_u._s.length += *bytes_read;
    return 0;
}
