#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "string.h"

#define STRING_CFS_SIZE 32

static int
defprintf(struct string* s, const char const* fmt, va_list exactlyonearg)
{
    char buf[4096];
    int len;

    len = vsnprintf(buf, 4096, fmt, exactlyonearg);
    return string_concatb(s, buf, len);
}

int string_concat_sprintf(struct string* s, const char *msg, ...)
{
    va_list args;
    char* a_s;
    struct string* a_S;
    char curchar;
    int a_i;
    char cfs[STRING_CFS_SIZE]; /* complex format string */
    int cfsptr;
    /* reserve one byte for the format char, one for the terminating zero */
    int cfsmaxlen = STRING_CFS_SIZE - 2;

    *cfs = '%';
    cfs[cfsmaxlen] = 0;

    va_start(args, msg);
    for (;*msg;msg++) {
        if (*msg == '%') {
            cfsptr = 1;
            switch((curchar = *++msg)) {
            case '\0':
                goto finished;

            case 's':
                a_s = va_arg(args, char*);
                if (string_concat(s, a_s)) return 1;
                break;

            case 'S':
                a_S = va_arg(args, struct string*);
                if (string_concats(s, a_S)) return 1;
                break;

            case 'd':
                a_i = va_arg(args, int);
                if (string_putint(s, a_i)) return 1;
                break;

            case '0' ... '9':
            case '.':
            case ' ':
            case '#':
            case '+':
            case  '-':
            case '\'': do {
                if (cfsptr == cfsmaxlen) {
                    (void)fprintf(stderr, "Debug: Format string \"%s\" exceeds maximum length!\n", cfs);
                    exit(1);
                }
                cfs[cfsptr++] = curchar;
            } while ((curchar = *++msg), ((curchar >= '0') && (curchar <= '9')) ||
                     (curchar == '.') || (curchar == ' ')   ||
                     (curchar == '#') || (curchar == '+')   ||
                     (curchar == '-') || (curchar == '\''));
            /* fallthrough */
            default:
                /* guaranteed to have two bytes left */
                cfs[cfsptr++] = curchar;
                cfs[cfsptr] = 0;
                if (defprintf(s, cfs, args)) return 1;
            }
        } else {
            if (string_putc(s, *msg)) return 1;
        }
    }
finished:
    va_end(args);

    if (string_putc(s, 0)) return 1;
    --s->_u._s.length;

    return 0;
}
