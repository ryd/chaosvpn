#ifndef __STRING_H__
#define __STRING_H__
struct string {
    union __u__ {
        char align[16];
        struct __s__ {
            int length;
            int size;
            int growby;
        } _s;
    } _u;
    char* s;
};

void string_clear(struct string*);
int string_concat(struct string*, const char*);
int string_concatb(struct string*, const char*, int);
void string_free(struct string*);
char* string_get(struct string*);
int string_init(struct string*, int, int);
#endif
