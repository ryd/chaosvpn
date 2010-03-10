#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/param.h>

#include "main.h"
#include "string/string.h"
#include "list.h"
#include "fs.h"
#include "tinc.h"
#include "settings.h"
#include "strnatcmp.h"

#define CONCAT(buffer, value)	if (string_concat(buffer, value)) return 1
#define CONCAT_F(buffer, format, value)	if (string_concat_sprintf(buffer, format, value)) return 1
#define CONCAT_DF(buffer, format, value, default_value)	if (string_concat_sprintf(buffer, format, str_is_nonempty(value) ? value : default_value)) return 1
#define CONCAT_YN(buffer, format, value, default_value)    if (string_concat_sprintf(buffer, format, str_is_true(value, default_value) ? "yes" : "no")) return 1
#define CONCAT_SN(buffer, value)	if (tinc_add_subnet(buffer, value)) return 1

static int tinc_add_subnet(struct string*, struct list_head*);

static bool
tinc_check_if_excluded(char *peername)
{
	struct list_head* ptr;
	struct settings_list* etr;

	if (s_exclude == NULL) {
		return false;
	}

	list_for_each(ptr, &s_exclude->list) {
		etr = list_entry(ptr, struct settings_list, list);
		if (etr->e->etype != LIST_STRING) {
			/* only strings allowed */
			continue;
		}
		if (strcasecmp(etr->e->evalue.s, peername) == 0) {
			return true;
		}
	}
	
	return false;
}

static int
tinc_generate_peer_config(struct string* buffer, struct peer_config *peer)
{
	if (str_is_nonempty(peer->gatewayhost)) {
		CONCAT_F(buffer, "Address=%s\n", peer->gatewayhost);
	}

	CONCAT_DF(buffer, "Cipher=%s\n", peer->cipher, TINC_DEFAULT_CIPHER);
	CONCAT_DF(buffer, "Compression=%s\n", peer->compression, TINC_DEFAULT_COMPRESSION);
	CONCAT_DF(buffer, "Digest=%s\n", peer->digest, TINC_DEFAULT_DIGEST);
	CONCAT_YN(buffer, "IndirectData=%s\n", peer->indirectdata, false);
	CONCAT_DF(buffer, "Port=%s\n", peer->port, TINC_DEFAULT_PORT);

	CONCAT_SN(buffer, &peer->network);
	CONCAT_SN(buffer, &peer->network6);

	CONCAT_YN(buffer, "TCPonly=%s\n", peer->use_tcp_only, false);
	CONCAT_F(buffer, "%s\n", peer->key);	

	return 0;
}

int
tinc_write_hosts(struct config *config)
{
	struct list_head *p = NULL;
	struct string hostfilepath;

	string_init(&hostfilepath, 512, 512);
	string_concat(&hostfilepath, config->base_path);
	string_concat(&hostfilepath, "/hosts/");

	fs_mkdir_p(string_get(&hostfilepath), 0700);

	list_for_each(p, &config->peer_config) {
		struct string peer_config;
		struct peer_config_list *i = container_of(p, 
				struct peer_config_list, list);

		printf("Writing config file for peer %s:", i->peer_config->name);
		(void)fflush(stdout);

		if (string_init(&peer_config, 2048, 512)) return 1;

		if (tinc_generate_peer_config(&peer_config, i->peer_config)) {
			string_free(&peer_config);
			return 1;
		}

		if (fs_writecontents_safe(string_get(&hostfilepath), 
				i->peer_config->name, string_get(&peer_config),
				string_length(&peer_config), 0600)) {
			fputs("unable to write host config file.\n", stderr);
			string_free(&peer_config);
			return 1;
		}

		string_free(&peer_config);
		(void)puts(".");
	}

	string_free(&hostfilepath);
	
	return 0;
}

int
tinc_write_config(struct config *config)
{
	struct list_head *p;
	struct string configfilename;
	struct string buffer;

	(void)fputs("Writing global config file:", stdout);
	(void)fflush(stdout);

	string_init(&buffer, 8192, 2048);


	/* generate main tinc.conf */

	CONCAT(&buffer, "AddressFamily=ipv4\n");

#ifndef BSD
	CONCAT(&buffer, "Device=/dev/net/tun\n");
	CONCAT_F(&buffer, "Interface=%s_vpn\n", config->networkname);
#endif

	CONCAT(&buffer, "Mode=router\n");
	CONCAT_F(&buffer, "Name=%s\n", config->peerid);
	CONCAT(&buffer, "Hostnames=no\n");
	CONCAT(&buffer, "PingTimeout=10\n");

	if (strnatcmp(config->tincd_version, "1.0.12") > 0) {
		/* this option is only available since 1.0.12+git / 1.0.13 */
		CONCAT(&buffer, "StrictSubnets=yes\n");
	} else {
		CONCAT(&buffer, "TunnelServer=yes\n");
	}

	if (str_is_nonempty(config->tincd_graphdumpfile)) {
		CONCAT_F(&buffer, "GraphDumpFile=%s\n", config->tincd_graphdumpfile);
	}

	if (str_is_nonempty(config->my_ip) && 
			strcmp(config->my_ip, "127.0.0.1") &&
			strcmp(config->my_ip, "0.0.0.0")) {
		CONCAT_F(&buffer, "BindToAddress=%s\n", config->my_ip);
	}

	if (str_is_true(config->my_peer->silent, false)) {
			return 0; //no ConnectTo lines
	}

	list_for_each(p, &config->peer_config) {
		struct peer_config_list *i = container_of(p, struct peer_config_list, list);

		if (!strcmp(i->peer_config->name, config->peerid)) {
			continue;
		}

		if (tinc_check_if_excluded(i->peer_config->name)) {
			continue;
		}

		if (str_is_nonempty(i->peer_config->gatewayhost) &&
				!str_is_true(i->peer_config->hidden, false)) {
			CONCAT_F(&buffer, "ConnectTo=%s\n", i->peer_config->name);
		}
	}


	/* write tinc.conf */

	string_init(&configfilename, 512, 512);
	string_concat(&configfilename, config->base_path);
	string_concat(&configfilename, "/tinc.conf");

	if (fs_writecontents(string_get(&configfilename), string_get(&buffer),
			string_length(&buffer), 0600)) {
		(void)fputs("unable to write tinc config file!\n", stderr);
		string_free(&buffer);
		string_free(&configfilename);
		return 1;
	}

	string_free(&buffer);
	string_free(&configfilename);

	(void)puts(".");

	return 0;
}

int
tinc_write_updown(struct config *config, bool up)
{
	/* up == true:  generate tinc-up */
	/* up == false: generate tinc-down */

	struct list_head *p;
	struct list_head *sp;
	struct peer_config_list *i;
	struct string_list *si;
	struct string buffer;
	struct string filepath;
	const char *routecmd;

	/* generate contents */

	string_init(&buffer, 8192, 2048);
	CONCAT(&buffer, "#!/bin/sh\n\n");

	if (up) {
		if (str_is_nonempty(config->ifconfig) && str_is_nonempty(config->vpn_ip)) {
			CONCAT_F(&buffer, "%s\n", config->ifconfig);
		}

		if (str_is_nonempty(config->ifconfig6) && str_is_nonempty(config->vpn_ip6)) {
			CONCAT_F(&buffer, "%s\n", config->ifconfig6);
		}

		CONCAT(&buffer, "\n");
	}

	list_for_each(p, &config->peer_config) {
		i = container_of(p, struct peer_config_list, list);

		if (!strcmp(i->peer_config->name, config->peerid)) {
			continue;
		}

		if (tinc_check_if_excluded(i->peer_config->name)) {
			CONCAT_F(&buffer, "# excluded node: %s\n", i->peer_config->name);
			continue;
		}

		CONCAT_F(&buffer, "# node: %s\n", i->peer_config->name);

		if (up)
			routecmd = config->routeadd;
		else
			routecmd = config->routedel;
		if (str_is_nonempty(config->vpn_ip) && str_is_nonempty(routecmd)) {
			list_for_each(sp, &i->peer_config->network) {
				si = container_of(sp, struct string_list, list);
				if (string_concat_sprintf(&buffer, routecmd, si->text)) return 1;
				string_putc(&buffer, '\n');
			}
		}
		
		if (up)
			routecmd = config->routeadd6;
		else
			routecmd = config->routedel6;
		if (str_is_nonempty(config->vpn_ip6) && str_is_nonempty(routecmd)) {
			list_for_each(sp, &i->peer_config->network6) {
				si = container_of(sp, struct string_list, list);
				if (string_concat_sprintf(&buffer, routecmd, si->text)) return 1;
				string_putc(&buffer, '\n');
			}
		}
	}

	CONCAT(&buffer, "\nexit 0\n\n");


	/* write contents into file */

	string_init(&filepath, 512, 512);
	string_concat(&filepath, config->base_path);
	if (up)
		string_concat(&filepath, "/tinc-up");
	else
		string_concat(&filepath, "/tinc-down");
	if (fs_writecontents(string_get(&filepath), string_get(&buffer), string_length(&buffer), 0700)) {
		(void)fprintf(stderr, "unable to write to %s!\n", string_get(&filepath));
		string_free(&buffer);
		string_free(&filepath);
		return 1;
	}
	
	string_free(&buffer);
	string_free(&filepath);

	return 0;
}

static int
tinc_add_subnet(struct string* buffer, struct list_head *network)
{
	struct list_head *p;

	list_for_each(p, network) {
		struct string_list *i = container_of(p, struct string_list, list);
		CONCAT_F(buffer, "Subnet=%s\n", i->text);
	}

	return 0;
}

char *
tinc_get_version(struct config *config) {
	struct string tincd_output;
	char *retval = NULL;
	char cmd[1024];
	char *p;
	int res;
	
	string_init(&tincd_output, 1024, 512);
	snprintf(cmd, sizeof(cmd), "%s --version", config->tincd_bin);
	res = fs_backticks_exec(cmd, &tincd_output);
	if (string_putc(&tincd_output, 0)) goto bail_out;

	if (strncmp(string_get(&tincd_output), "tinc version ", 13) != 0) {
		retval = NULL;
		goto bail_out;
	}

	if ((p = strchr(string_get(&tincd_output)+13, ' '))) {
		*p = '\0';
	}
	
	//printf("tinc version output: '%s'\n", string_get(&tincd_output));

	retval = strdup(string_get(&tincd_output)+13);

	//printf("tinc version: '%s'\n", retval);

bail_out:
	string_free(&tincd_output);
	return retval;
}
