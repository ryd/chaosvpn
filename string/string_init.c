#include <stdlib.h>

#include "string.h"

int
string_init(struct string* s, size_t size, size_t growby)
{
    s->_u._s.length = 0;
    s->s = malloc(size);
    if (!s->s) return 1;
    s->_u._s.size = size;
    s->_u._s.growby = growby;
    return 0;
}
