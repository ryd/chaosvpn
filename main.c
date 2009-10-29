#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "main.h"
#include "tun.h"
#include "http.h"
#include "parser.h"
#include "tinc.h"

int main (int argc,char *argv[]) {
		if (tun_check_or_create()) {
			printf("Error - unable to create tun device - maybe wrong user\n");
			return 1;
		}

		struct buffer *http_response = malloc(sizeof(struct buffer));
		if (http_request("https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt?id=undef&password=&ip=", http_response)) {
			printf("Unable to fetch tinc-chaosvpn.txt - maybe server is down\n");
			return 1;
		}

		struct list_head config_list;
		INIT_LIST_HEAD(&config_list);

		if (parser_parse_config(http_response->text, &config_list)) {
			printf("Unable to parse config\n");
			return 1;
		}

		struct buffer *tinc_config = malloc(sizeof(struct buffer));
		tinc_generate_config(tinc_config, "chaos", "undef", "127.0.0.1");
		printf("config:\n%s", tinc_config->text);

		struct list_head *p;
		list_for_each(p, &config_list) {
			struct buffer *peer_config = malloc(sizeof(struct buffer));
			struct configlist *i = container_of(p, struct configlist, list);

			tinc_generate_peer_config(peer_config, i->config);
			printf("peer:\n%s", peer_config->text);
		}

		return 0;
}
