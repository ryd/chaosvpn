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
    if (s1 == s2) return 0;
    if (s1->s == s2->s) return 0;
    if (s1->_u._s.length != s2->_u._s.length) return 1;

    if (s1->s == NULL || s2->s == NULL)
        return 1;

    if (memcmp(string_get(s1), string_get(s2), s1->_u._s.length) != 0)
        return 1;
    return 0;
}
