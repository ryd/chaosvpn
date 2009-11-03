#include <stdlib.h>
#include <string.h>

#include "string.h"

int string_concat(struct string* s, const char* sta)
{
    if (string_concatb(s, sta, strlen(sta))) return 1;
    if (string_concatb(s, "\0", 1)) return 1;
    s->_u._s.length--;
    return 0;
}
