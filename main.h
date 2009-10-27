#include "list.h"

struct buffer {
	char *text;
};

struct stringlist {
    struct list_head list;
    char *text;
};

struct configlist {
    struct list_head list;
    struct config *config;
};

struct config {
    char *name;
    char *gatewayhost;
    char *owner;
    char *use_tcp_only;
    struct list_head network;
    struct list_head network6;
    struct list_head route_network;
    struct list_head route_network6;
    char *hidden;
    char *silent;
    char *port;
    char *indirectdata;
    char *key;
};

