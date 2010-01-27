#include <stdlib.h>
#include <string.h>

#include "string.h"

/**
 * checks if two strings are equal.
 * @returns 0 if the two strings are equal, otherwise nonzero
 */
int
string_equals(struct string* s1, struct string* s2)
{
    uintptr_t i;

    if (s1 == s2) return 0;
    if (s1->s == s2->s) return 0;
    if (s1->_u._s.length != s2->_u._s.length) return 1;
    
    for (i = 0; i < s1->_u._s.length; i++) if (s1->s[i] != s2->s[i]) return 1;
    return 0;
}
