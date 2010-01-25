#ifndef __STRING_H__
#define __STRING_H__

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


/* FUNCTIONS WORKING WITH struct string */
/* ------------------------------------ */

struct string {
    union __u__ {
        char align[16];
        struct __s__ {
            uintptr_t length;
            uintptr_t size;
            uintptr_t growby;
        } _s;
    } _u;
    char* s;
};

void string_clear(struct string*);
int string_concat(struct string*, const char*);
int string_concatb(struct string*, const char*, uintptr_t);
int string_concat_sprintf(struct string* s, const char *msg, ...);
int string_putc(struct string*, char);
int string_putint(struct string*, int);
void string_free(struct string*);
char* string_get(struct string*);
int string_init(struct string*, uintptr_t, uintptr_t);

static inline uintptr_t string_length(struct string *s) {
    return s->_u._s.length;
}


/* FUNCTIONS WORKING WITH PLAIN char* */
/* ---------------------------------- */

static inline bool str_is_empty(char *s) {
    if (s != NULL && strlen(s) > 0) {
        return false;
    } else {
        return true;
    }
}
static inline bool str_is_nonempty(char *s) {
    if (s != NULL && strlen(s) > 0) {
        return true;
    } else {
        return false;
    }
}

static inline bool str_is_true(char *s, bool def) {
    if (str_is_nonempty(s)) {
        if (strcasecmp(s, "0")==0 || strcasecmp(s,"no")==0) {
            return false;
        } else if (strcasecmp(s, "1")==0 || strcasecmp(s,"yes")==0) {
            return true;
        } else {
            return def; /* TODO: something better needed */
        }
    } else {
        return def;
    }
}

#endif
