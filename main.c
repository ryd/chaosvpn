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

extern FILE *yyin;

static int main_check_root() {
	return getuid() != 0;
}

int main (int argc,char *argv[]) {
	struct buffer *http_response = NULL;
	struct list_head config_list;
	struct buffer *tinc_config = NULL;
	struct list_head *p = NULL;

	struct string httpurl;
	struct string configfilename;
	struct string hostfilepath;

	if (main_check_root()) {
		printf("Error - wrong user - please start as root user\n");
		return 1;
	}

	yyin = fopen("chaosvpn.conf", "r");
	if (!yyin) {
		fputs("Error: unable to open chaosvpn.conf!\n", stderr);
		return 1;
	}
	yyparse();

	if (tun_check_or_create()) {
		printf("Error - unable to create tun device\n");
		return 1;
	}

	(void)fputs("Fetching information:", stdout);
	(void)fflush(stdout);

	http_response = calloc(1, sizeof *http_response);
	if (http_response == NULL) {
		printf("Unable to allocate memory.\n");
		return 1;
	}

	string_init(&httpurl, 512, 512);
	string_concat(&httpurl, s_master_url);
	string_concat(&httpurl, "?id=");
	string_concat(&httpurl, s_my_peerid);
	string_concat(&httpurl, "&password=");
	string_concat(&httpurl, s_my_password);
	if (http_request(string_get(&httpurl), http_response)) {
		printf("Unable to fetch tinc-chaosvpn.txt - maybe server is down\n");
		return 1;
	}
	string_free(&httpurl);
	puts(".");

	INIT_LIST_HEAD(&config_list);

	if (parser_parse_config(http_response->text, &config_list)) {
		printf("Unable to parse config\n");
		free(http_response);
		return 1;
	}

	(void)fputs("Writing global config file:", stdout);
	(void)fflush(stdout);

	tinc_config = malloc(sizeof *tinc_config);
	tinc_generate_config(tinc_config, s_networkname, s_my_peerid, s_my_vpn_ip, &config_list);

	string_init(&configfilename, 512, 512);
	string_concat(&configfilename, s_my_peerid);
	string_concat(&configfilename, ".config");
	if(fs_writecontents(string_get(&configfilename), tinc_config->text, strlen(tinc_config->text), 0600)) {
		(void)fputs("unable to write config file!\n", stderr);
		free(tinc_config);
		return 1;
	}
	string_free(&configfilename);

	tinc_config = malloc(sizeof *tinc_config);
	tinc_generate_up(tinc_config, s_networkname, s_networkname, s_my_vpn_ip, &config_list);
	if(fs_writecontents("up.sh", tinc_config->text, strlen(tinc_config->text), 0600)) {
		(void)fputs("unable to write up file!\n", stderr);
		free(tinc_config);
		return 1;
	}
	(void)puts(".");

	string_init(&hostfilepath, 512, 512);
	string_concat(&hostfilepath, s_my_peerid);
	string_concat(&hostfilepath, "/hosts/");
	list_for_each(p, &config_list) {
		struct buffer *peer_config;
		struct configlist *i = container_of(p, struct configlist, list);

		peer_config = malloc(sizeof *peer_config);

		printf("Writing config file for peer %s:", i->config->name);
		(void)fflush(stdout);
		tinc_generate_peer_config(peer_config, i->config);
		if(fs_writecontents_safe(string_get(&hostfilepath), i->config->name, peer_config->text, strlen(peer_config->text), 0600)) {
			fputs("unable to write config file.\n", stderr);
			free(peer_config);
			return 1;
		}
		(void)puts(".");
	}
	string_free(&hostfilepath);

	return 0;
}
