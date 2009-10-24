#include <stdlib.h>
#include <stdio.h>
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
		if (parser_parse_config(my_buffer.text)) {
			printf("Unable to parse config\n");
			return 1;
		}
		return 0;
}
