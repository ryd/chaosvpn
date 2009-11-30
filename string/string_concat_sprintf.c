#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "string.h"

int string_concat_sprintf(struct string* s, const char *msg, ...)
{
    va_list args;
    char* a_s;
    int a_i;

    va_start(args, msg);
    for (;*msg;msg++) {
        if (*msg == '%') {
            switch(*++msg) {
            case 's':
                a_s = va_arg(args, char*);
                if (string_concat(s, a_s)) return 1;
                break;

            case 'd':
                a_i = va_arg(args, int);
                if (string_putint(s, a_i)) return 1;
                break;

            default:
                (void)fprintf(stderr, "Debug: Format entity %%%c not defined yet, implement it, please!\n", *msg);
                exit(1);
            }
        } else {
            if (string_putc(s, *msg)) return 1;
        }
    }

    if (string_putc(s, 0)) return 1;

    return 0;
}
