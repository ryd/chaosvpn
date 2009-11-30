#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "list.h"
#include "tinc.h"


static int tinc_add_subnet(struct string*, struct list_head*);

int
tinc_generate_peer_config(struct string* buffer, struct peer_config *peer)
{
	if (strlen(peer->gatewayhost) > 0) {
		if (string_concat_sprintf(buffer, "Address=%s\n", peer->gatewayhost)) return 1;
	}

	if (string_concat_sprintf(buffer, "Cipher=%s\n", strlen(peer->cipher) > 0 ? peer->cipher : TINC_DEFAULT_CIPHER)) return 1;
	if (string_concat_sprintf(buffer, "Compression=%s\n", strlen(peer->compression) > 0 ? peer->compression : TINC_DEFAULT_COMPRESSION)) return 1;
	if (string_concat_sprintf(buffer, "Digest=%s\n", strlen(peer->digest) > 0 ? peer->digest : TINC_DEFAULT_DIGEST)) return 1;
	if (string_concat_sprintf(buffer, "IndirectData=%s\n", strlen(peer->indirectdata) == 0 ? "no" : "yes")) return 1;
	if (string_concat_sprintf(buffer, "Port=%s\n", strlen(peer->port) > 0 ? peer->port : TINC_DEFAULT_PORT)) return 1;

	if (tinc_add_subnet(buffer, &peer->network)) return 1;
	if (tinc_add_subnet(buffer, &peer->network6)) return 1;

	if (string_concat_sprintf(buffer, "TCPonly=%s\n", strlen(peer->use_tcp_only) == 0 ? "no" : "yes")) return 1;

	if (string_concat(buffer, peer->key)) return 1;
	if (string_concat(buffer, "\n")) return 1;

	return 0;
}

int
tinc_generate_config(struct string* buffer, struct config *config)
{
	struct list_head *p;

	if (string_concat(buffer, "AddressFamily=ipv4\n")) return 1;
	if (string_concat(buffer, "Device=/dev/net/tun\n\n")) return 1;

	if (string_concat_sprintf(buffer, "Interface=%s_vpn\n", config->networkname)) return 1;
	if (string_concat(buffer, "Mode=router\n")) return 1;
	if (string_concat_sprintf(buffer, "Name=%s\n", config->peerid)) return 1;
	if (string_concat(buffer, "Hostnames=yes\n")) return 1;

	if (config->vpn_ip && strlen(config->vpn_ip) > 0 && 
			!strcmp(config->vpn_ip, "127.0.0.1")) {
		if (string_concat_sprintf(buffer, "BindToAddress=%s\n", config->vpn_ip)) return 1;
	}

	list_for_each(p, &config->peer_config) {
        struct peer_config_list *i = container_of(p, struct peer_config_list, list);

		if (strlen(i->peer_config->gatewayhost) > 0 &&
				strlen(i->peer_config->hidden) == 0 &&
				strlen(i->peer_config->silent) == 0) {
			if (string_concat_sprintf(buffer, "ConnectTo=%s\n", i->peer_config->name)) return 1;
		}
	}

	return 0;
}

int
tinc_generate_up(struct string* buffer, struct config *config)
{
	struct list_head *p;
	struct list_head *sp;
	struct peer_config_list *i;
	struct string_list *si;

	if (config->ifconfig != NULL) {
		if (string_concat_sprintf(buffer, "%s\n", config->ifconfig)) return 1;
	}
	if (config->ifconfig6 != NULL) {
		if (string_concat_sprintf(buffer, "%s\n", config->ifconfig6)) return 1;
	}
	list_for_each(p, &config->peer_config) {
		i = container_of(p, struct peer_config_list, list);

		if (strlen(i->peer_config->gatewayhost) > 0) {
	        	list_for_each(sp, &i->peer_config->network) {
				si = container_of(sp, struct string_list, list);
				if (string_concat_sprintf(buffer, "/sbin/ip -4 route add %s dev $INTERFACE\n", si->text)) return 1;
		        }
	        	list_for_each(sp, &i->peer_config->network6) {
		                si = container_of(sp, struct string_list, list);
				if (string_concat_sprintf(buffer, "/sbin/ip -6 route add %s dev $INTERFACE\n", si->text)) return 1;
		        }
		}
	}

	return 0;
}

static int
tinc_add_subnet(struct string* buffer, struct list_head *network)
{
	struct list_head *p;

	list_for_each(p, network) {
		struct string_list *i = container_of(p, struct string_list, list);

		if (string_concat_sprintf(buffer, "Subnet=%s\n", i->text)) return 1;
	}

	return 0;
}
