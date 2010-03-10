#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "main.h"
#include "config.h"
#include "settings.h"


struct config*
config_alloc(void)
{
	struct config *config;

	config = malloc(sizeof(struct config));
	if (config == NULL) return NULL;
	memset(config, 0, sizeof(struct config));

	config->peerid			= NULL;
	config->vpn_ip			= NULL;
	config->vpn_ip6			= NULL;
	config->networkname		= NULL;
	config->my_ip			= NULL;
	config->tincd_bin		= "/usr/sbin/tincd";
	config->routeadd		= NULL;
	config->routeadd6		= NULL;
	config->routedel		= NULL;
	config->routedel6		= NULL;
	config->ifconfig		= NULL;
	config->ifconfig6		= NULL; // not required
	config->master_url		= "https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt";
	config->base_path		= "/etc/tinc";
	config->pidfile			= "/var/run/chaosvpn.default.pid";
	config->my_peer			= NULL;
	config->masterdata_signkey	= NULL;
	config->tincd_graphdumpfile	= NULL;
	config->ifmodifiedsince = 0;

	string_lazyinit(&config->privkey, 2048);
	INIT_LIST_HEAD(&config->peer_config);

	config->configfile		= "/etc/tinc/chaosvpn.conf";
	config->donotfork		= false;

	return config;
}
