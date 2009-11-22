#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "list.h"
#include "tinc.h"

char *tinc_extent_string(char *dest, char *src) {
	char *text = realloc(dest, strlen(dest) + strlen(src) + 1);
	strcat(text, src);
	return text;
}

char *tinc_add_subnet(char *config, struct list_head *network) {
	struct list_head *p;

	list_for_each(p, network) {
		struct string_list *i = container_of(p, struct string_list, list);

		config = tinc_extent_string(config, "Subnet=");
		config = tinc_extent_string(config, i->text);
		config = tinc_extent_string(config, "\n");
	}

	return config;
}

int tinc_generate_peer_config(struct buffer *output, struct peer_config *peer) {
	char *config = calloc(sizeof(char), 1); //empty string

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
			"TCPonly=no\n" :
			"TCPonly=yes\n"
	);

	config = tinc_extent_string(config, peer->key);
	config = tinc_extent_string(config, "\n");

	output->text = config;
	return 0;
}

int tinc_generate_config(struct buffer *output, struct config *config) {
	struct list_head *p;
	char *buffer = calloc(sizeof(char), 1);

	buffer = tinc_extent_string(buffer, "AddressFamily=ipv4\n");
	buffer = tinc_extent_string(buffer, "Device=/dev/net/tun\n\n");

	buffer = tinc_extent_string(buffer, "Interface=");
	buffer = tinc_extent_string(buffer, config->networkname);
	buffer = tinc_extent_string(buffer, "_vpn\n");

	buffer = tinc_extent_string(buffer, "Mode=router\n");

	buffer = tinc_extent_string(buffer, "Name=");
	buffer = tinc_extent_string(buffer, config->peerid);
	buffer = tinc_extent_string(buffer, "\n");

	buffer = tinc_extent_string(buffer, "Hostnames=yes\n");

	if (config->vpn_ip && strlen(config->vpn_ip) > 0 && 
			!strcmp(config->vpn_ip, "127.0.0.1")) {
		buffer = tinc_extent_string(buffer, "BindToAddress=");
		buffer = tinc_extent_string(buffer, config->vpn_ip);
		buffer = tinc_extent_string(buffer, "\n");
	}

    list_for_each(p, &config->peer_config) {
        struct peer_config_list *i = container_of(p, struct peer_config_list, list);

		if (strlen(i->peer_config->gatewayhost) > 0 &&
				strlen(i->peer_config->hidden) == 0 &&
				strlen(i->peer_config->silent) == 0) {
			buffer = tinc_extent_string(buffer, "ConnectTo=");
			buffer = tinc_extent_string(buffer, i->peer_config->name);
			buffer = tinc_extent_string(buffer, "\n");
		}
	}

	output->text = buffer;

	return 0;
}

int tinc_generate_up(struct buffer *output, struct config *config) {
	struct list_head *p;
	char *buffer = calloc(sizeof(char), 1);

	buffer = tinc_extent_string(buffer, "#!/bin/sh\n");

    list_for_each(p, &config->peer_config) {
        struct peer_config_list *i = container_of(p, struct peer_config_list, list);

		if (strlen(i->peer_config->gatewayhost) > 0) {
			buffer = tinc_extent_string(buffer, "/bin/ip -4 route add ");
			buffer = tinc_extent_string(buffer, i->peer_config->name);
			buffer = tinc_extent_string(buffer, " dev $INTERFACE\n");
		}
	}

	// TODO still not complete

	output->text = buffer;
	return 0;
}

