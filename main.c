#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include "fs.h"
#include "list.h"
#include "main.h"
#include "tun.h"
#include "http.h"
#include "parser.h"
#include "tinc.h"
#include "settings.h"

#include "string/string.h"

#define CONFIG_FILE "chaosvpn.conf"

extern FILE *yyin;

static int main_check_root() {
	return getuid() != 0;
}

struct config* main_initialize_config() {
	struct config *config = malloc(sizeof(config));

    config->peerid		= NULL;
    config->vpn_ip		= NULL;
    config->vpn_ip6		= NULL;
    config->networkname = NULL;
    config->tincd_bin	= "/usr/sbin/tincd";
    config->ip_bin		= "/sbin/ip";
    config->ifconfig	= "/sbin/ifconfig";
    config->ifconfig6	= NULL; // not required
    config->master_url	= "https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt";
    config->base_path	= "/etc/tinc";
    config->pidfile		= "/var/run";

	INIT_LIST_HEAD(&config->peer_config);

	return config;
}

int main_init(struct config *config) {
	if (main_check_root()) {
		printf("Error - wrong user - please start as root user\n");
		return 1;
	}

	if (tun_check_or_create()) {
		printf("Error - unable to create tun device\n");
		return 1;
	}

	yyin = fopen(CONFIG_FILE, "r");
	if (!yyin) {
		fputs("Error: unable to open chaosvpn.conf!\n", stderr);
		return 1;
	}
	yyparse();

	// optional params
	if (s_master_url != NULL)	config->master_url	= s_master_url;
	if (s_tincd_bin != NULL)	config->tincd_bin	= s_tincd_bin;
	if (s_ip_bin != NULL)		config->ip_bin		= s_ip_bin;
	if (s_ifconfig != NULL)		config->ifconfig	= s_ifconfig;
	if (s_base != NULL)			config->base_path	= s_base;
	if (s_pidfile != NULL)		config->pidfile		= s_pidfile;

	//require params
	if (s_my_peerid == NULL) {
		printf("peerid is missing in %s\n", CONFIG_FILE);
		return 1;
	}
	config->peerid = s_my_peerid;

	if (s_networkname == NULL) {
		printf("networkname is missing in %s\n", CONFIG_FILE);
		return 1;
	}
	config->networkname = s_networkname;

	return 0;
}

int main_request_config(struct config *config, struct buffer *http_response) {
	struct string httpurl;

	string_init(&httpurl, 512, 512);
	string_concat(&httpurl, config->master_url);
	string_concat(&httpurl, "?id=");
	string_concat(&httpurl, config->peerid);
	//string_concat(&httpurl, "&password=");
	//string_concat(&httpurl, config->password);

	if (http_request(string_get(&httpurl), http_response)) {
		printf("Unable to fetch %s - maybe server is down\n", config->master_url);
		string_free(&httpurl);
		return 1;
	}

	string_free(&httpurl);

	return 0;
}

int main_parse_config(struct config *config, struct buffer *http_response) {
	if (parser_parse_config(http_response->text, &config->peer_config)) {
		printf("Unable to parse config\n");
		return 1;
	}

	return 0;
}

int main_write_config_tinc(struct config *config) {
	struct string configfilename;
	struct buffer *tinc_config = malloc(sizeof *tinc_config);

	tinc_generate_config(tinc_config, config);

	string_init(&configfilename, 512, 512);
	string_concat(&configfilename, config->peerid);
	string_concat(&configfilename, ".config");

	if(fs_writecontents(string_get(&configfilename), tinc_config->text, 
			strlen(tinc_config->text), 0600)) {
		(void)fputs("unable to write tinc config file!\n", stderr);
		free(tinc_config);
		return 1;
	}

	free(tinc_config);
	string_free(&configfilename);

	return 0;
}

int main_write_config_hosts(struct config *config) {
	struct list_head *p = NULL;
	struct string hostfilepath;

	string_init(&hostfilepath, 512, 512);
	string_concat(&hostfilepath, config->peerid);
	string_concat(&hostfilepath, "/hosts/");

	fs_mkdir_p(string_get(&hostfilepath), 0700);

	list_for_each(p, &config->peer_config) {
		struct buffer *peer_config;
		struct peer_config_list *i = container_of(p, 
				struct peer_config_list, list);

		peer_config = malloc(sizeof *peer_config);

		printf("Writing config file for peer %s:", i->peer_config->name);
		(void)fflush(stdout);

		tinc_generate_peer_config(peer_config, i->peer_config);

		if(fs_writecontents_safe(string_get(&hostfilepath), 
				i->peer_config->name, peer_config->text, 
				strlen(peer_config->text), 0600)) {
			fputs("unable to write host config file.\n", stderr);
			free(peer_config);
			return 1;
		}

		free(peer_config);
		(void)puts(".");
	}

	string_free(&hostfilepath);
	
	return 0;
}

int main_write_config_up(struct config *config) {
	struct buffer *up_file = malloc(sizeof(struct buffer));

	tinc_generate_up(up_file, config);

	// TODO path for up.sh
	if(fs_writecontents("up.sh", up_file->text, strlen(up_file->text), 0600)) {
		(void)fputs("unable to write up file!\n", stderr);
		free(up_file);
		return 1;
	}
	
	free(up_file);

	return 0;
}

int main_create_backup(struct config *config) {
	// TODO to be implementated
	return 0;
}

int main (int argc,char *argv[]) {
	struct config *config = main_initialize_config();

	int err = main_init(config);
	if (err) return err;

	(void)fputs("Fetching information:", stdout);
	(void)fflush(stdout);

	struct buffer *http_response = calloc(1, sizeof *http_response);
	if (http_response == NULL) {
		printf("Unable to allocate memory.\n");
		return 1;
	}

	err = main_request_config(config, http_response);
	if (err) return err;

	err = main_parse_config(config, http_response);
	if (err) return err;

	free(http_response);

	puts(".");
	(void)fputs("Writing global config file:", stdout);
	(void)fflush(stdout);

	if (main_create_backup(config) ||
			main_write_config_tinc(config) ||
			main_write_config_hosts(config) ||
			main_write_config_up(config)) {
		return 1;
	}

	// TODO pidfile check && tincd start

	(void)puts(".");

	return 0;
}
