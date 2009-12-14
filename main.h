#include "list.h"

struct buffer {
	char *text;
};

struct string_list {
    struct list_head list;
    char *text;
};

struct peer_config {
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
    char *cipher;
    char *compression;
    char *digest;
};

struct peer_config_list {
    struct list_head list;
    struct peer_config *peer_config;
};

struct config {
	char *peerid;
	char *vpn_ip;
	char *vpn_ip6;
	char *networkname;
	char *tincd_bin;
	char *ip_bin;
	char *ifconfig;
	char *ifconfig6;
	char *master_url;
	char *base_path;
	char *pidfile;
	struct peer_config *my_peer;
	struct list_head peer_config;
};

