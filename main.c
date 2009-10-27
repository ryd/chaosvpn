#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "main.h"

struct buffer my_buffer;

int main (int argc,char *argv[]) {
		if (tun_check_or_create()) {
			printf("Error - unable to create tun device - maybe wrong user\n");
			return 1;
		}

		char buffer[8196];
		struct buffer *buffer_ptr = &my_buffer;
		if (http_request("https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt?id=undef&password=&ip=", buffer_ptr)) {
			printf("Unable to fetch tinc-chaosvpn.txt - maybe server is down\n");
			return 1;
		}

		struct list_head config_list;
		INIT_LIST_HEAD(&config_list);

		if (parser_parse_config(my_buffer.text, &config_list)) {
			printf("Unable to parse config\n");
			return 1;
		}

		struct list_head *p;
		list_for_each(p, &config_list) {
			struct configlist *i = container_of(p, struct configlist, list);
			printf("name:%s\n", i->config->name);
			printf("key:%s", i->config->key);
		}

		return 0;
}
