#include <stdlib.h>
#include <stdbool.h>

#include "string.h"

bool
string_init(struct string* s, size_t size, size_t growby)
{
    s->length = 0;
    s->growby = growby;
    s->s = malloc(size);
    if (s->s == NULL) {
        s->size = 0;
    } else {
        memset(s->s, 0, size);
        s->size = size;
    }
    return true;
}
