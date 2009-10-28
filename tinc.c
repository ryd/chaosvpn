#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

#define TINC_DEFAULT_PORT "665"

char *tinc_extent_string(char *dest, char *src) {
	char *text = realloc(dest, strlen(dest) + strlen(src) + 1);
	strcat(text, src);
	return text;
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

