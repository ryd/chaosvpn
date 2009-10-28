#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "list.h"

#define TINC_DEFAULT_PORT "665"

char *tinc_extent_string(char *dest, char *src) {
	char *text = realloc(dest, strlen(dest) + strlen(src) + 1);
	strcat(text, src);
	return text;
}

char *tinc_add_subnet(char *config, struct list_head *network) {
	struct list_head *p;
	list_for_each(p, network) {
		struct stringlist *i = container_of(p, struct stringlist, list);

		config = tinc_extent_string(config, "Subnet=");
		config = tinc_extent_string(config, i->text);
		config = tinc_extent_string(config, "\n");
	}

	return config;
}

int tinc_generate_peer_config(struct buffer *output, struct config *peer) {
	char *config = malloc(sizeof(char));
	memcpy(config, "\0", 1); //empty string

	config = tinc_extent_string(config, "Address=");
	config = tinc_extent_string(config, peer->gatewayhost);
	config = tinc_extent_string(config, "\n");

	config = tinc_extent_string(config, "Cipher=blowfish\n");
	config = tinc_extent_string(config, "Compression=0\n");
	config = tinc_extent_string(config, "Digest=sha1\n");

	config = tinc_extent_string(config,
			strlen(peer->indirectdata) == 0 ?
			"IndirectData=no\n" :
			"IndirectData=yes\n"
	);

	config = tinc_extent_string(config, "Port=");
	config = tinc_extent_string(config,
			strlen(peer->port) > 0 ?
			peer->port :
			TINC_DEFAULT_PORT
	);
	config = tinc_extent_string(config, "\n");

	config = tinc_add_subnet(config, &peer->network);
	config = tinc_add_subnet(config, &peer->network6);

	config = tinc_extent_string(config,
			strlen(peer->use_tcp_only) == 0 ?
			"TCPonly=no\n\n" :
			"TCPonly=yes\n\n"
	);

	config = tinc_extent_string(config, peer->key);

	output->text = config;
	return 0;
}

int tinc_generate_config(struct buffer *output, char *interface, char *name, char *ip) {
	char *config = malloc(sizeof(char));
	memcpy(config, "\0", 1); //empty string

	config = tinc_extent_string(config, "AddressFamily=ipv4\n");
	config = tinc_extent_string(config, "Device=/dev/net/tun\n\n");

	config = tinc_extent_string(config, "Interface=");
	config = tinc_extent_string(config, interface);
	config = tinc_extent_string(config, "_vpn\n");

	config = tinc_extent_string(config, "Mode=router\n");

	config = tinc_extent_string(config, "Name=");
	config = tinc_extent_string(config, name);
	config = tinc_extent_string(config, "\n");

	config = tinc_extent_string(config, "BindToAddress=");
	config = tinc_extent_string(config, ip);
	config = tinc_extent_string(config, "\n");

	output->text = config;
	return 0;
}

