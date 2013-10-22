#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "string.h"

/**
 * checks if two strings are equal.
 * @returns true if the two strings are equal, otherwise false
 */
bool
string_equals(struct string* s1, struct string* s2)
{
    if (s1 == s2) return true;
    if (s1->s == s2->s) return true;
    if (s1->_u._s.length != s2->_u._s.length) return false;

    if (s1->s == NULL || s2->s == NULL)
        return false;

    if (memcmp(string_get(s1), string_get(s2), s1->_u._s.length) != 0)
        return false;
    return true;
}
