#include <stdlib.h>

#include "string.h"

void
string_lazyinit(struct string* s, uintptr_t growby)
{
    s->length = 0;
    s->s = NULL;
    s->size = 0;
    s->growby = growby;
}
