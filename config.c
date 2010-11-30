#include <ctype.h>
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
	config->pidfile			= NULL;
	config->cookiefile		= NULL;
	config->my_peer			= NULL;
	config->masterdata_signkey	= NULL;
	config->tincd_graphdumpfile	= NULL;
	config->tincd_device		= NULL;
	config->tincd_interface		= NULL;
	config->tincd_user		= NULL;
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

int
config_init(struct config *config)
{
	struct stat st; 
	struct string privkey_name;
	char tmp[1024];
	char *p;
	struct stat stat_buf;
	struct passwd* pwentry;

	globalconfig = config;

	yyin = fopen(config->configfile, "r");
	if (!yyin) {
		log_err("Error: unable to open %s\n", config->configfile);
		return 1;
	}
	yyparse();
	fclose(yyin);

	if ((config->update_interval == 0) && (!config->oneshot)) {
		log_err("Error: you have not configured a remote config update interval.");
		log_err("($update_interval) Please configure an interval (3600 - 7200 seconds");
		log_err("are recommended) or activate legacy (cron) mode by using the -a flag.");
		exit(1);
	}
	if ((config->update_interval < 60) && (!config->oneshot)) {
		log_err("Error: $update_interval may not be <60.");
		exit(1);
	}


	// check required params
	#define reqparam(paramfield, label) if (str_is_empty(config->paramfield)) { \
		log_err("%s is missing or empty in %s\n", label, config->configfile); \
		return 1; \
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
		return 1;
	}

    /* Note on the beauty of the POSIX API:
     * although pwentry is dynamically allocated here (maybe),
     * it /must not/ be freed after use. */
	pwentry = getpwnam(config->tincd_user);
	if (!pwentry) {
		log_err("tincd_user %s does not exist.", config->tincd_user);
		return 1;
	}
	config->tincd_uid = pwentry->pw_uid;
	config->tincd_gid = pwentry->pw_gid;

	if (strcmp(config->vpn_ip, "172.31.0.255") == 0) {
		log_err("error: you have to change $my_vpn_ip in %s\n", config->configfile);
		return 1;
	}

	if (config->use_dynamic_routes)
		reqparam(routedel, "$routedel");

	if (str_is_nonempty(config->vpn_ip6)) {
		reqparam(ifconfig6, "$ifconfig6");
		reqparam(routeadd6, "$routeadd6");
		
		if (config->use_dynamic_routes)
			reqparam(routedel6, "$routedel6");
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
	if (stat(config->base_path, &st) & fs_mkdir_p(config->base_path, 0750, 0, config->tincd_gid)) {
		log_err("error: unable to mkdir %s\n", config->base_path);
		return 1;
	}

	string_init(&privkey_name, 1024, 512);
	if (string_concat_sprintf(&privkey_name, "%s/rsa_key.priv", config->base_path)) { return 1; }

	string_free(&config->privkey); /* just to be sure */
	if (fs_read_file(&config->privkey, string_get(&privkey_name))) {
		log_err("error: can't read private rsa key at %s\n", string_get(&privkey_name));
		string_free(&privkey_name);
		return 1;
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
	if (config->pidfile == NULL) {
		snprintf(tmp, sizeof(tmp), "/var/run/tinc.%s.pid", config->networkname);
		config->pidfile = strdup(tmp);
	}


	if (config->tincd_version &&
		(strnatcmp(config->tincd_version, "1.1") > 0)) {
		/* tinc 1.1-git does not use a pid file anymore */

		snprintf(tmp, sizeof(tmp), "%s", config->pidfile);
		p = strrchr(tmp, '.');
		if ((p != NULL) && (strcasecmp(p, ".pid") == 0)) {
			/* replace trailing .pid with .cookie */
			int len = p - tmp;
			
			snprintf(tmp+len, sizeof(tmp)-len, "%s", ".cookie");
		}
		
		config->cookiefile = strdup(tmp);

		free(config->pidfile);
		config->pidfile = NULL;
	} else {
		/* tinc 1.0.x does not use a cookie file */
		free(config->cookiefile);
		config->cookiefile = NULL;
	}

	return 0;
}

/* get pointer to already allocated and initialized config structure */
struct config*
config_get(void)
{
	return globalconfig;
}
