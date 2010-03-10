#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "list.h"
#include "string/string.h"
#include "main.h"
#include "config.h"
#include "settings.h"
#include "fs.h"


extern FILE *yyin;


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

int
config_init(struct config *config)
{
	struct stat st; 
	struct string privkey_name;

	yyin = fopen(config->configfile, "r");
	if (!yyin) {
		(void)fprintf(stderr, "Error: unable to open %s\n", config->configfile);
		return 1;
	}
	yyparse();
	fclose(yyin);

	if ((s_update_interval == 0) && (!config->donotfork)) {
		(void)fputs("Error: you have not configured a remote config update interval.\n" \
					"($update_interval) Please configure an interval (3600 - 7200 seconds\n" \
					"are recommended) or activate legacy (cron) mode by using the -a flag.\n", stderr);
		exit(1);
	}
	if ((s_update_interval < 60) && (!config->donotfork)) {
		(void)fputs("Error: $update_interval may not be <60.\n", stderr);
		exit(1);
	}


	// first copy all parsed params into config structure
	if (s_my_peerid != NULL)		config->peerid			= s_my_peerid;
	if (s_my_vpn_ip != NULL)		config->vpn_ip			= s_my_vpn_ip;
	if (s_my_vpn_ip6 != NULL)		config->vpn_ip6			= s_my_vpn_ip6;
	if (s_networkname != NULL)		config->networkname		= s_networkname;
	if (s_my_ip != NULL)			config->my_ip			= s_my_ip;
	if (s_tincd_bin != NULL)		config->tincd_bin		= s_tincd_bin;
	if (s_routeadd != NULL)			config->routeadd		= s_routeadd;
	if (s_routeadd6 != NULL)		config->routeadd6		= s_routeadd6;
	if (s_routedel != NULL)			config->routedel		= s_routedel;
	if (s_routedel6 != NULL)		config->routedel6		= s_routedel6;
	if (s_ifconfig != NULL)			config->ifconfig		= s_ifconfig;
	if (s_ifconfig6 != NULL)		config->ifconfig6		= s_ifconfig6;
	if (s_master_url != NULL)		config->master_url		= s_master_url;
	if (s_base != NULL)			config->base_path		= s_base;
	if (s_masterdata_signkey != NULL)	config->masterdata_signkey	= s_masterdata_signkey;
	if (s_tincd_graphdumpfile != NULL)	config->tincd_graphdumpfile	= s_tincd_graphdumpfile;
	if (s_pidfile != NULL)			config->pidfile			= s_pidfile;

	// then check required params
	#define reqparam(paramfield, label) if (str_is_empty(config->paramfield)) { \
		fprintf(stderr, "%s is missing or empty in %s\n", label, config->configfile); \
		return 1; \
		}

	reqparam(peerid, "$my_peerid");
	reqparam(networkname, "$networkname");
	reqparam(vpn_ip, "$my_vpn_ip");
	reqparam(routeadd, "$routeadd");
	reqparam(routedel, "$routedel");
	reqparam(ifconfig, "$ifconfig");
	reqparam(base_path, "$base");


	// create base directory
	if (stat(config->base_path, &st) & fs_mkdir_p(config->base_path, 0700)) {
		fprintf(stderr, "error: unable to mkdir %s\n", config->base_path);
		return 1;
	}

	string_init(&privkey_name, 1024, 512);
	if (string_concat_sprintf(&privkey_name, "%s/rsa_key.priv", config->base_path)) { return 1; }

	string_free(&config->privkey); /* just to be sure */
	if (fs_read_file(&config->privkey, string_get(&privkey_name))) {
		fprintf(stderr, "error: can't read private rsa key at %s\n", string_get(&privkey_name));
		string_free(&privkey_name);
		return 1;
	}
	string_free(&privkey_name);

	return 0;
}
