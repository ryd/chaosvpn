#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "chaosvpn.h"


extern FILE *yyin;
extern int yyparse(void);

struct config *globalconfig = NULL;


static void
free_settings_list(struct settings_list *s)
{
	struct list_head *pos, *next;

	if (s == NULL)
		return;

	list_for_each_safe(pos, next, &s->list) {
		struct settings_list* etr;
		
		etr = list_entry(pos, struct settings_list, list);
		list_del(&etr->list);
		if (etr->e->etype == LIST_STRING) {
			free(etr->e->evalue.s);
		}
		free(etr);
	}
	free(s);
}

struct config*
config_alloc(void)
{
	struct config *config;

	config = malloc(sizeof(struct config));
	if (config == NULL) return NULL;
	memset(config, 0, sizeof(struct config));

	config->pidfile			= NULL;
	config->peerid			= NULL;
	config->vpn_ip			= NULL;
	config->vpn_ip6			= NULL;
	config->networkname		= NULL;
	config->my_ip			= NULL;
	config->my_addressfamily	= NULL;
	config->tincd_bin		= strdup("/usr/sbin/tincd");
	config->tincctl_bin		= NULL;
	config->tincd_debuglevel	= 3;
	config->tincd_restart_delay	= 20;
	config->routemetric		= strdup("0");
	config->routeadd		= NULL;
	config->routeadd6		= NULL;
	config->routedel		= NULL;
	config->routedel6		= NULL;
	config->ifconfig		= NULL;
	config->ifconfig6		= NULL; // not required
	config->master_url		= strdup("https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt");
	config->base_path		= strdup("/etc/tinc");
	config->tincd_pidfile		= NULL;
	config->my_peer			= NULL;
	config->masterdata_signkey	= NULL;
	config->tincd_graphdumpfile	= NULL;
	config->tincd_device		= NULL;
	config->tincd_interface		= NULL;
	config->tincd_user		= NULL;
	config->tincd_raw_config	= NULL;
	config->use_dynamic_routes	= false;
	config->connect_only_to_primary_nodes = true;
	config->update_interval		= 0;
	config->ifmodifiedsince		= 0;

	string_lazyinit(&config->privkey, 2048);
	INIT_LIST_HEAD(&config->peer_config);

	config->configfile		= strdup("/etc/tinc/chaosvpn.conf");
	config->daemonmode		= false;
	config->oneshot			= false;

	config->tincd_uid		= 0;
	config->tincd_gid		= 0;

	return config;
}

void
config_free(struct config *config)
{
	if (config == NULL) return;

	free(config->pidfile);
	free(config->peerid);
	free(config->vpn_ip);
	free(config->vpn_ip6);
	free(config->networkname);
	free(config->my_ip);
	free(config->my_addressfamily);
	free(config->tincd_bin);
	free(config->tincctl_bin);
	free(config->routemetric);
	free(config->routeadd);
	free(config->routeadd6);
	free(config->routedel);
	free(config->routedel6);
	free(config->postup);
	free(config->ifconfig);
	free(config->ifconfig6);
	free(config->master_url);
	free(config->base_path);
	free(config->tincd_pidfile);
	free(config->masterdata_signkey);
	free(config->tincd_graphdumpfile);
	free(config->tmpconffile);
	free(config->tincd_device);
	free(config->tincd_interface);
	free(config->tincd_user);
	free(config->tincd_raw_config);
	free(config->vpn_netmask);
	free(config->password);

	string_free(&config->privkey);

	free_settings_list(config->exclude);
	parser_free_config(&config->peer_config);
	free(config->configfile);
	free(config->tincd_version);
	free_settings_list(config->mergeroutes_supernet_raw);
	addrmask_free(config->mergeroutes_supernet);
	free_settings_list(config->ignore_subnets_raw);
	addrmask_free(config->ignore_subnets);
	free_settings_list(config->whitelist_subnets_raw);
	addrmask_free(config->whitelist_subnets);

	if (globalconfig == config) {
		globalconfig = NULL;
	}

	free(config);
}

static struct addr_info *
parse_and_free_raw_subnetlist(struct settings_list *raw, const char *label)
{
	struct list_head *ptr;
	struct settings_list* etr;
	struct addr_info *current;
	struct addr_info *prev;
	struct addr_info *addrlist;

	if (!raw)
		return NULL;

	addrlist = NULL;
	prev = NULL;

	list_for_each(ptr, &raw->list) {
		etr = list_entry(ptr, struct settings_list, list);
		if (etr->e->etype != LIST_STRING) {
			/* only strings allowed */
			continue;
		}

		current = addrmask_init(etr->e->evalue.s);
		if (!current) {
			log_err("%s: invalid ip/mask '%s' - ignored.", label, etr->e->evalue.s);
			continue;
		}

		if (!addrlist) {
			addrlist = current;
		} else if (prev) {
			prev->next = current;
		}

		prev = current;
	}

	free_settings_list(raw);

	return addrlist;
}

bool
config_init(struct config *config)
{
	struct stat st; 
	struct string privkey_name;
	char tmp[1024];
	struct stat stat_buf;
	struct passwd* pwentry;

	globalconfig = config;

	yyin = fopen(config->configfile, "r");
	if (!yyin) {
		log_err("Error: unable to open %s\n", config->configfile);
		return false;
	}
	yyparse();
	fclose(yyin);

	if ((config->update_interval == 0) && (!config->oneshot)) {
		log_err("Error: you have not configured a remote config update interval.");
		log_err("($update_interval) Please configure an interval (3600 - 7200 seconds");
		log_err("are recommended) or activate legacy (cron) mode by using the -o flag.");
		exit(1);
	}
	if ((config->update_interval < 60) && (!config->oneshot)) {
		log_err("Error: $update_interval may not be <60.");
		exit(1);
	}


	// check required params
	#define reqparam(paramfield, label) if (str_is_empty(config->paramfield)) { \
		log_err("%s is missing or empty in %s\n", label, config->configfile); \
		return false; \
		}

	reqparam(peerid, "$my_peerid");
	reqparam(networkname, "$networkname");
	reqparam(vpn_ip, "$my_vpn_ip");
	reqparam(routeadd, "$routeadd");
	reqparam(ifconfig, "$ifconfig");
	reqparam(base_path, "$base");
	reqparam(tincd_user, "$tincd_user");
	reqparam(tincd_bin, "$tincd_bin");

	if (stat(config->tincd_bin, &stat_buf) ||
		(!(stat_buf.st_mode & S_IXUSR))) {
		log_err("tinc binary %s not executable.", config->tincd_bin);
		return false;
	}

	if (str_is_empty(config->my_ip) ||
		strcmp(config->my_ip, "0.0.0.0") == 0 ||
		strcmp(config->my_ip, "127.0.0.1") == 0 ||
		strcmp(config->my_ip, "::") == 0) {

		free(config->my_ip);
		config->my_ip = NULL;
	}
	if (str_is_nonempty(config->my_ip) && !addrmask_verify_ip(config->my_ip, AF_UNSPEC)) {
		log_err("no valid ip address found in $my_ip");
		return false;
	}

	if (str_is_nonempty(config->my_addressfamily) &&
		strcmp(config->my_addressfamily, "any") != 0 &&
		strcmp(config->my_addressfamily, "ipv4") != 0 &&
		strcmp(config->my_addressfamily, "ipv6") != 0) {

		log_err("invalid setting for $my_addressfamily, only 'ipv4', 'ipv6' or 'any' allowed.");
		return false;
	}
	if (str_is_empty(config->my_addressfamily)) {
		free(config->my_addressfamily);
		config->my_addressfamily = strdup("ipv4");
	}

	/* Note on the beauty of the POSIX API:
	 * although pwentry is dynamically allocated here (maybe),
	 * it /must not/ be freed after use. */
	pwentry = getpwnam(config->tincd_user);
	if (!pwentry) {
		log_err("tincd_user %s does not exist.", config->tincd_user);
		return false;
	}
	config->tincd_uid = pwentry->pw_uid;
	config->tincd_gid = pwentry->pw_gid;

	if (str_is_nonempty(config->vpn_ip) && !addrmask_verify_ip(config->vpn_ip, AF_INET)) {
		log_err("no valid ipv4 address found in $my_vpn_ip");
		return false;
	}
	if (strcmp(config->vpn_ip, "172.31.0.255") == 0) {
		log_err("error: you have to change $my_vpn_ip in %s\n", config->configfile);
		return false;
	}

	if (str_is_nonempty(config->vpn_ip6) && !addrmask_verify_ip(config->vpn_ip6, AF_INET6)) {
		log_err("no valid ipv6 address found in $my_vpn_ip6");
		return false;
	}

	if (config->use_dynamic_routes)
		reqparam(routedel, "$routedel");

	if (str_is_nonempty(config->vpn_ip6)) {
		reqparam(ifconfig6, "$ifconfig6");
		reqparam(routeadd6, "$routeadd6");
		
		if (config->use_dynamic_routes)
			reqparam(routedel6, "$routedel6");
	}

	if (config->mergeroutes_supernet)
		addrmask_free(config->mergeroutes_supernet);
	config->mergeroutes_supernet =
		parse_and_free_raw_subnetlist(config->mergeroutes_supernet_raw, "@mergeroutes_supernet");
	config->mergeroutes_supernet_raw = NULL;
	if (config->mergeroutes_supernet && config->use_dynamic_routes) {
		log_err("settings @mergeroutes_supernet and $use_dynamic_routes are not compatible!");
		log_err("disable one of them and retry.");
		return false;
	}

	if (config->ignore_subnets)
		addrmask_free(config->ignore_subnets);
	config->ignore_subnets =
		parse_and_free_raw_subnetlist(config->ignore_subnets_raw, "@ignore_subnets");
	config->ignore_subnets_raw = NULL;
	if (config->ignore_subnets && config->use_dynamic_routes) {
		log_err("settings @ignore_subnets and $use_dynamic_routes are not compatible!");
		log_err("disable one of them and retry.");
		return false;
	}

	if (config->whitelist_subnets)
		addrmask_free(config->whitelist_subnets);
	config->whitelist_subnets =
		parse_and_free_raw_subnetlist(config->whitelist_subnets_raw, "@whitelist_subnets");
	config->whitelist_subnets_raw = NULL;
	if (config->whitelist_subnets && config->use_dynamic_routes) {
		log_err("settings @whitelist_subnets and $use_dynamic_routes are not compatible!");
		log_err("disable one of them and retry.");
		return false;
	}


#ifndef BSD
	/* Linux */
	if (str_is_empty(config->tincd_device)) {
		free(config->tincd_device);
		config->tincd_device = strdup("/dev/net/tun");
	}
	if (str_is_empty(config->tincd_interface)) {
		free(config->tincd_interface);
		snprintf(tmp, sizeof(tmp), "%s_vpn", config->networkname);
		config->tincd_interface = strdup(tmp);
	}
#else
	/* BSD */
	if (str_is_nonempty(config->tincd_interface) && str_is_empty(config->tincd_device)) {
		free(config->tincd_device);
		snprintf(tmp, sizeof(tmp), "/dev/%s", config->tincd_interface);
		config->tincd_device = strdup(tmp);
	}
#endif

	// create base directory
	if (stat(config->base_path, &st) & fs_mkdir_p(config->base_path, 0700)) {
		log_err("error: unable to mkdir %s\n", config->base_path);
		return false;
	}

	string_init(&privkey_name, 1024, 512);
	if (string_concat_sprintf(&privkey_name, "%s/rsa_key.priv", config->base_path)) { return 1; }

	string_free(&config->privkey); /* just to be sure */
	if (fs_read_file(&config->privkey, string_get(&privkey_name))) {
		log_err("error: can't read private rsa key at %s\n", string_get(&privkey_name));
		string_free(&privkey_name);
		return false;
	}
	string_free(&privkey_name);


	config->tincd_version = tinc_get_version(config);
	if (config->tincd_version == NULL) {
		log_warn("Warning: can't determine tinc version!\n");
	}


	/* tincctl only exists since tinc 1.1-git */
	if (config->tincd_version &&
		(strnatcmp(config->tincd_version, "1.1") > 0)) {
		if (config->tincctl_bin == NULL) {
			char *slash;
			char tincctl[1024] = "";
			int len = 0;

			slash = strrchr(config->tincd_bin, '/');
			if (slash != NULL) {
				len = slash - config->tincd_bin + 1;
				if (len > sizeof(tincctl)-1) {
					/* ugly, but unlikely to happen */
					len = sizeof(tincctl)-1;
				}
				strncpy(tincctl, config->tincd_bin, len);
				tincctl[len] = '\0';
			}
			snprintf(tincctl+len, sizeof(tincctl)-len, "tincctl");

			config->tincctl_bin = strdup(tincctl);
		}
	} else {
		free(config->tincctl_bin);
		config->tincctl_bin = NULL;
	}

	/* setup pidfile name if not defined in configfile */
	if (config->tincd_pidfile == NULL) {
		snprintf(tmp, sizeof(tmp), "/var/run/tinc.%s.pid", config->networkname);
		config->tincd_pidfile = strdup(tmp);
	}

	/* setup tmpconffile if not defined in configfile */
	if (config->tmpconffile == NULL) {
		snprintf(tmp, sizeof(tmp), "%s/data.sav", config->base_path);
		config->tmpconffile = strdup(tmp);
	}

	return true;
}

/* get pointer to already allocated and initialized config structure */
struct config*
config_get(void)
{
	return globalconfig;
}
