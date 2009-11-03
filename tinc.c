#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "list.h"
#include "tinc.h"
#include "string/string.h"

static int tinc_extent_string(struct string* config, char *src) {
    return string_concat(config, src);
}

int tinc_add_subnet(struct string* config, struct list_head *network) {
	struct list_head *p;
	list_for_each(p, network) {
		struct stringlist *i = container_of(p, struct stringlist, list);

		if(tinc_extent_string(config, "Subnet=")) return 1;
		if(tinc_extent_string(config, i->text)) return 1;
		if(tinc_extent_string(config, "\n")) return 1;
	}

    return 0;
}

int tinc_generate_peer_config(struct buffer *output, struct config *peer) {
    struct string config;
    
    string_init(&config, 512, 1024);

	if(tinc_extent_string(&config, "Address=")) return 1;
	if(tinc_extent_string(&config, peer->gatewayhost)) return 1;
	if(tinc_extent_string(&config, "\n")) return 1;

	if(tinc_extent_string(&config, "Cipher=blowfish\n")) return 1;
	if(tinc_extent_string(&config, "Compression=0\n")) return 1;
	if(tinc_extent_string(&config, "Digest=sha1\n")) return 1;

	if(tinc_extent_string(&config,
			strlen(peer->indirectdata) == 0 ?
			"IndirectData=no\n" :
			"IndirectData=yes\n"
	)) return 1;

	if(tinc_extent_string(&config, "Port=")) return 1;
	if(tinc_extent_string(&config,
			strlen(peer->port) > 0 ?
			peer->port :
			TINC_DEFAULT_PORT
	)) return 1;
	if(tinc_extent_string(&config, "\n")) return 1;

	if(tinc_add_subnet(&config, &peer->network)) return 1;
	if(tinc_add_subnet(&config, &peer->network6)) return 1;

	if(tinc_extent_string(&config,
			strlen(peer->use_tcp_only) == 0 ?
			"TCPonly=no\n\n" :
			"TCPonly=yes\n\n"
	)) return 1;

	if(tinc_extent_string(&config, peer->key)) return 1;

	output->text = config.s;
	return 0;
}

int tinc_generate_config(struct buffer *output, char *interface, char *name, char *ip) {
    struct string config;

    string_init(&config, 512, 1024);

	if(tinc_extent_string(&config, "AddressFamily=ipv4\n")) return 1;
	if(tinc_extent_string(&config, "Device=/dev/net/tun\n\n")) return 1;

	if(tinc_extent_string(&config, "Interface=")) return 1;
	if(tinc_extent_string(&config, interface)) return 1;
	if(tinc_extent_string(&config, "_vpn\n")) return 1;

	if(tinc_extent_string(&config, "Mode=router\n")) return 1;

	if(tinc_extent_string(&config, "Name=")) return 1;
	if(tinc_extent_string(&config, name)) return 1;
	if(tinc_extent_string(&config, "\n")) return 1;

	if(tinc_extent_string(&config, "BindToAddress=")) return 1;
	if(tinc_extent_string(&config, ip)) return 1;
	if(tinc_extent_string(&config, "\n")) return 1;

	output->text = config.s;
	return 0;
}

