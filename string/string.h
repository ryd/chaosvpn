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
void string_move(struct string*, struct string*);
int string_equals(struct string*, struct string*);
void string_lazyinit(struct string*, uintptr_t);
int string_initfromstringz(struct string*, const char const*);


static inline uintptr_t string_length(struct string *s) {
    /* amount of bytes filled with content */
    return s->_u._s.length;
}

static inline uintptr_t string_size(struct string *s) {
    /* amount of memory (in bytes) currently allocated */
    return s->_u._s.size;
}

static inline int
string_concats(struct string* s, struct string* ss)
{
    return string_concatb(s, ss->s, ss->_u._s.length);
}

static inline int
string_ensurez(struct string* s)
{
    if (s->s)
        if (s->s[s->_u._s.length - 1] == 0)
            return 0;
    if (string_putc(s, 0)) return 1;
    --s->_u._s.length;
    return 0;
}

static inline void
string_mkstatic(struct string* s, char* z)
{
    s->s = z;
    s->_u._s.growby = 0;
    s->_u._s.size = strlen(z);
    s->_u._s.length = strlen(z);
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
        if (strcmp(s, "0")==0 || strcasecmp(s,"no")==0) {
            return false;
        } else if (strcmp(s, "1")==0 || strcasecmp(s,"yes")==0) {
            return true;
        } else {
            return def; /* TODO: something better needed */
        }
    } else {
        return def;
    }
}

#endif
