#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "list.h"
#include "tinc.h"
#include "settings.h"

#define CONCAT(buffer, value)	if (string_concat(buffer, value)) return 1
#define CONCAT_F(buffer, format, value)	if (string_concat_sprintf(buffer, format, value)) return 1
#define CONCAT_DF(buffer, format, value, default_value)	if (string_concat_sprintf(buffer, format, strlen(value) > 0 ? value : default_value)) return 1
#define CONCAT_YN(buffer, format, value)	if (string_concat_sprintf(buffer, format, strlen(value) == 0 ? "no" : "yes")) return 1
#define CONCAT_SN(buffer, value)	if (tinc_add_subnet(buffer, value)) return 1

static int tinc_add_subnet(struct string*, struct list_head*);

int
tinc_generate_peer_config(struct string* buffer, struct peer_config *peer)
{
	CONCAT_DF(buffer, "Address=%s\n", peer->gatewayhost, "");

	CONCAT_DF(buffer, "Cipher=%s\n", peer->cipher, TINC_DEFAULT_CIPHER);
	CONCAT_DF(buffer, "Compression=%s\n", peer->compression, TINC_DEFAULT_COMPRESSION);
	CONCAT_DF(buffer, "Digest=%s\n", peer->digest, TINC_DEFAULT_DIGEST);
	CONCAT_YN(buffer, "IndirectData=%s\n", peer->indirectdata);
	CONCAT_DF(buffer, "Port=%s\n", peer->port, TINC_DEFAULT_PORT);

	CONCAT_SN(buffer, &peer->network);
	CONCAT_SN(buffer, &peer->network6);

	CONCAT_YN(buffer, "TCPonly=%s\n", peer->use_tcp_only);
	CONCAT_F(buffer, "%s\n", peer->key);	

	return 0;
}

int
tinc_generate_config(struct string* buffer, struct config *config)
{
	struct list_head *p;

	CONCAT(buffer, "AddressFamily=ipv4\n");
	CONCAT(buffer, "Device=/dev/net/tun\n\n");

	CONCAT_F(buffer, "Interface=%s_vpn\n", config->networkname);
	CONCAT(buffer, "Mode=router\n");
	CONCAT_F(buffer, "Name=%s\n", config->peerid);
	CONCAT(buffer, "Hostnames=yes\n");

	if (config->vpn_ip && strlen(config->vpn_ip) > 0 && 
			!strcmp(config->vpn_ip, "127.0.0.1")) {
		CONCAT_F(buffer, "BindToAddress=%s\n", config->vpn_ip);
	}

	list_for_each(p, &config->peer_config) {
		struct peer_config_list *i = container_of(p, struct peer_config_list, list);
		if (!strcmp(i->peer_config->name, s_my_peerid)) {
			continue;
		}
		if (strlen(i->peer_config->gatewayhost) > 0 &&
				strlen(i->peer_config->hidden) == 0 &&
				strlen(i->peer_config->silent) == 0) {
			CONCAT_F(buffer, "ConnectTo=%s\n", i->peer_config->name);
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
		CONCAT_F(buffer, "%s\n", config->ifconfig);
	}

	if (config->ifconfig6 != NULL) {
		CONCAT_F(buffer, "%s\n", config->ifconfig6);
	}

	list_for_each(p, &config->peer_config) {
		i = container_of(p, struct peer_config_list, list);

		if (!strcmp(i->peer_config->name, config->peerid)) {
			continue;
		}
		if (strlen(i->peer_config->gatewayhost) > 0) {
			list_for_each(sp, &i->peer_config->network) {
				si = container_of(sp, struct string_list, list);
				CONCAT_F(buffer, "/sbin/ip -4 route add %s dev $INTERFACE\n", si->text);
			}
			list_for_each(sp, &i->peer_config->network6) {
				si = container_of(sp, struct string_list, list);
				CONCAT_F(buffer, "/sbin/ip -6 route add %s dev $INTERFACE\n", si->text);
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
		CONCAT_F(buffer, "Subnet=%s\n", i->text);
	}

	return 0;
}
