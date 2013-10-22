#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "string.h"

#define MAXSTRINGLEN 32

// ugly
bool string_putint(struct string* s, int i)
{
    char buf[MAXSTRINGLEN];
    int len;

    len = snprintf(buf, MAXSTRINGLEN, "%d", i);

    if (len >= (MAXSTRINGLEN - 1)) {
        (void)fputs("error: increase MAXSTRINGLEN in string_putint\n", stderr);
        exit(1);
    }

    return string_concatb(s, buf, len);
}
