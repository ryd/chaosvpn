#include <stdlib.h>

#include "string.h"

void
string_lazyinit(struct string* s, uintptr_t growby)
{
    s->_u._s.length = 0;
    s->s = NULL;
    s->_u._s.size = 0;
    s->_u._s.growby = growby;
}
