#include <stdlib.h>
#include <stdbool.h>

#include "string.h"

bool
string_initfromstringz(struct string* s, const char *is)
{
    size_t l;

    l = strlen(is);
    string_lazyinit(s, l);
    return string_concatb(s, is, l);
}
