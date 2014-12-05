#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "string.h"

bool
string_concat(struct string* s, const char* sta)
{
    if (!string_concatb(s, sta, strlen(sta) + 1)) return false;
    s->length--;
    return true;
}
