#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "main.h"

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
		tinc_generate_config(tinc_config);
		printf("config: %s\n", tinc_config->text);

		return 0;
}
