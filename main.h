#ifndef __MAIN_H
#define __MAIN_H

#include "list.h"

struct string_list {
    struct list_head list;
    char *text;
};

#endif
