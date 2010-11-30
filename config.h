#ifndef __CONFIG_H
#define __CONFIG_H

#include <time.h>
#include "string/string.h"
#include "list.h"
#include "main.h"

typedef enum E_settings_list_entry_type {
	LIST_STRING,
	LIST_INTEGER,
	LIST_LIST /* reserved */
} settings_list_entry_type;

struct settings_list_entry {
	settings_list_entry_type etype;
	union {
		char* s;
		int i;
	} evalue;
};

struct settings_list {
	struct list_head list;
	struct settings_list_entry* e;
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
	char *primary;
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
	char *my_ip;
	char *tincd_bin;
	char *tincd_version;
	char *tincctl_bin;
	unsigned int tincd_debuglevel;
	unsigned int tincd_restart_delay;
	char *routemetric;
	char *routeadd;
	char *routeadd6;
	char *routedel;
	char *routedel6;
	char *ifconfig;
	char *ifconfig6;
	char *master_url;
	char *base_path;
	char *pidfile;
	char *cookiefile;
	char *masterdata_signkey;
	char *tincd_graphdumpfile;
	char *tmpconffile;
	char *tincd_device;
	char *tincd_interface;
	char *tincd_user;
	struct string privkey;
	struct settings_list *exclude;
	struct peer_config *my_peer;
	struct list_head peer_config;
	time_t ifmodifiedsince;
	unsigned int update_interval;
	bool use_dynamic_routes;
	bool connect_only_to_primary_nodes;

	/* vars only used in configfile, dummy for c code: */
	char *password;
	char *vpn_netmask;

	/* commandline parameter: */
	char *configfile;
	bool daemonmode;
	bool oneshot;

    /* used to set various file permissions */
    uid_t tincd_uid;
    gid_t tincd_gid;
};

extern struct config* config_alloc(void);
extern int config_init(struct config *config);

/* get pointer to already allocated and initialized config structure */
extern struct config* config_get(void);

#endif
