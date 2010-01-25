#include <stdlib.h>

#include "string.h"

int
string_init(struct string* s, uintptr_t size, uintptr_t growby)
{
    s->_u._s.length = 0;
    s->s = malloc(size);
    if (!s->s) {
        s->_u._s.size = 0;
        s->_u._s.growby = growby;
    } else {
        s->_u._s.size = size;
        s->_u._s.growby = growby;
    }
    return 0;
}
