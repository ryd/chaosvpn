#include <stdlib.h>
#include <string.h>

#include "string.h"

/**
 * Moves a string from one struct string to another. The old struct
 * will be cleared and can be reused. If it is not reused, there is
 * no need to free it.
 */
void
string_move(struct string* s1, struct string* s2)
{
    memcpy(s2, s1, sizeof(struct string));
    s1->s = NULL;
    s1->_u._s.size = 0;
    s1->_u._s.length = 0;
}
