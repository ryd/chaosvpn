#include <stdlib.h>

#include "string.h"

int
string_initfromstringz(struct string* s, const char const* is)
{
    uintptr_t l;

    l = strlen(is);
    string_lazyinit(s, l);
    return string_concatb(s, is, l);
}
