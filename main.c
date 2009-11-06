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

static bool main_check_root() {
		    return getuid() != 0;
}

int main (int argc,char *argv[]) {
	struct buffer *http_response = NULL;
	struct list_head config_list;
	struct buffer *tinc_config = NULL;
	struct list_head *p = NULL;

		if (main_check_root()) {
			printf("Error - wrong user - please start as root user\n");
			return 1;
		}

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

		if (http_request("https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt?id=undef&password=&ip=", http_response)) {
			printf("Unable to fetch tinc-chaosvpn.txt - maybe server is down\n");
			return 1;
		}
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
		tinc_generate_config(tinc_config, "chaos", "undef", "127.0.0.1");
		if(fs_writecontents("undef.config", tinc_config->text, strlen(tinc_config->text), 0400)) {
			(void)fputs("unable to write config file!\n", stderr);
			free(tinc_config);
			return 1;
        	}
		(void)puts(".");

		list_for_each(p, &config_list) {
			struct buffer *peer_config;
			struct configlist *i = container_of(p, struct configlist, list);

			peer_config = malloc(sizeof *peer_config);

			printf("Writing config file for peer %s:", i->config->name);
			(void)fflush(stdout);
			tinc_generate_peer_config(peer_config, i->config);
			if(fs_writecontents_safe("undef/hosts/", i->config->name, peer_config->text, strlen(peer_config->text), 0400)) {
				fputs("unable to write config file.\n", stderr);
				free(peer_config);
				return 1;
			}
			(void)puts(".");
		}

		return 0;
}
