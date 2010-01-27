
#include <stdbool.h>

extern bool ar_is_ar_file(struct string *archive);
extern int ar_extract(struct string *archive, char *membername, struct string *result);

