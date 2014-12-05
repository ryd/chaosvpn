#include <stdlib.h>
#include <stdbool.h>

#include "string.h"

bool
string_init(struct string* s, size_t size, size_t growby)
{
    s->length = 0;
    s->s = malloc(size);
    if (!s->s) {
        s->size = 0;
        s->growby = growby;
    } else {
        memset(s->s, 0, size);
        s->size = size;
        s->growby = growby;
    }
    return true;
}
