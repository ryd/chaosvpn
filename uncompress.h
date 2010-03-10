#ifndef __UNCOMPRESS_H
#define __UNCOMPRESS_H

#include "string/string.h"

extern int uncompress_inflate(struct string *compressed, struct string *uncompressed);

#endif
