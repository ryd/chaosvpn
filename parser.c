#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "chaosvpn.h"

static struct peer_config *my_config = NULL;
static bool parse_key_mode;

static char*
parser_check_configitem(char *line, char *config)
{
	int len;

	len = strlen(config);
	if (!strncmp(line, config, len)) {
		return line + len;
	}
	return NULL;
}

static struct list_head*
parser_stringlist(char *item)
{
	struct string_list *i = malloc(sizeof(struct string_list));
	if (i == NULL) return NULL;
	memset(i, 0, sizeof(struct string_list));
	i->text = strdup(item);
	return &i->list;
}

static void
parser_extend_key(char *line)
{
	my_config->key = realloc(
			my_config->key, 
			strlen(my_config->key) + strlen(line) + 2
	);
	if (my_config->key == NULL) {
		log_err("parser_extend_key: realloc() failed!\n");
		exit(1);
	}
	strcat(my_config->key, line);
	strcat(my_config->key, "\n");
}

static bool
parser_create_config(char *name)
{
	my_config = malloc(sizeof(struct peer_config));
	if (my_config == NULL) return false;

	memset(my_config, 0, sizeof(struct peer_config));

	INIT_LIST_HEAD(&my_config->network);
	INIT_LIST_HEAD(&my_config->network6);
	INIT_LIST_HEAD(&my_config->route_network);
	INIT_LIST_HEAD(&my_config->route_network6);

	my_config->name = strdup(name);
	my_config->gatewayhost = strdup("");
	my_config->owner = strdup("");
	my_config->use_tcp_only = strdup("");
	my_config->hidden = strdup("");
	my_config->silent = strdup("");
	my_config->port = strdup("");
	my_config->indirectdata = strdup("");
	my_config->key = strdup("");
	my_config->cipher = strdup("");
	my_config->compression = strdup("");
	my_config->digest = strdup("");
	my_config->primary = strdup("");

	return true;
}

static void
parser_delete_string_list(struct list_head* list)
{
	struct list_head* i;
	struct list_head* tmp;
	struct string_list* item;

	list_for_each_safe(i, tmp, list) {
		item = container_of(i, struct string_list, list);
		free(item->text);
		list_del(i);
		free(item);
	}
}

static void
parser_free_peer_config(struct peer_config *config)
{
	free(config->name);
	free(config->gatewayhost);
	free(config->owner);
	free(config->use_tcp_only);
	free(config->hidden);
	free(config->silent);
	free(config->port);
	free(config->indirectdata);
	free(config->key);
	free(config->cipher);
	free(config->compression);
	free(config->digest);
	free(config->primary);
	parser_delete_string_list(&config->network);
	parser_delete_string_list(&config->network6);
	parser_delete_string_list(&config->route_network);
	parser_delete_string_list(&config->route_network6);
}

void
parser_free_config(struct list_head* configlist)
{
	struct list_head* i;
	struct list_head* tmp;
	struct peer_config_list* item;

	list_for_each_safe(i, tmp, configlist) {
		item = container_of(i, struct peer_config_list, list);
		parser_free_peer_config(item->peer_config);
		free(item->peer_config);
		list_del(i);
		free(item);
	}
	
}

static void
parser_replace_item(char **var, char *newitem)
{
	free(*var);
	*var = strdup(newitem);
}

static bool
parser_parse_line(char *line, struct list_head *configlist)
{
	int len;
	char *item;
	struct addr_info *addr;

	len = strlen(line);
	if ((*line == '[') && (line[len - 1] == ']')) {
		struct peer_config_list *i;
		parse_key_mode = false;

		item = calloc(sizeof(char), len - 1);
		if (item == NULL) {
			free(my_config);
			my_config = NULL;
			return false;
		}
		memcpy(item, line + 1, len - 2);
		item[len - 2] = 0;

		i = malloc(sizeof(struct peer_config_list));
		if (i == NULL) {
			free(my_config);
			my_config = NULL;
			free(item);
			return false;
		}

		memset(i, 0, sizeof(struct peer_config_list));

		parser_create_config(item);
		i->peer_config = my_config;
		list_add_tail(&i->list, configlist);
		free(item);
	} else if (my_config == NULL) {
		/* we did not start with a [...] header */
		/* and my_config is not allocated+initialized yet */
		/* skip until after first valid header initialized a config section */
		return true;
	} else if ((item = parser_check_configitem(line, "gatewayhost="))) {
		parser_replace_item(&my_config->gatewayhost, item);
	} else if ((item = parser_check_configitem(line, "owner="))) {
		parser_replace_item(&my_config->owner, item);
	} else if ((item = parser_check_configitem(line, "use-tcp-only="))) {
		parser_replace_item(&my_config->use_tcp_only, item);
	} else if ((item = parser_check_configitem(line, "network="))) {
		addr = addrmask_init(item);
		if (addr) {
			if (addr->addr_family == AF_INET)
				list_add_tail(parser_stringlist(item), &my_config->network);
			else
				log_err("node [%s]: not ipv4 for network='%s'", my_config->name, item);
			addrmask_free(addr);
		} else {
			log_err("node [%s]: received invalid network='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "network6="))) {
		addr = addrmask_init(item);
		if (addr) {
			if (addr->addr_family == AF_INET6)
				list_add_tail(parser_stringlist(item), &my_config->network6);
			else
				log_err("node [%s]: not ipv6 for network6='%s'", my_config->name, item);
			addrmask_free(addr);
		} else {
			log_err("node [%s]: received invalid network6='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "route_network="))) {
		addr = addrmask_init(item);
		if (addr) {
			if (addr->addr_family == AF_INET)
				list_add_tail(parser_stringlist(item), &my_config->route_network);
			else
				log_err("node [%s]: not ipv4 for network6='%s'", my_config->name, item);
			addrmask_free(addr);
		} else {
			log_err("node [%s]: received invalid route_network='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "route_network6="))) {
		addr = addrmask_init(item);
		if (addr) {
			if (addr->addr_family == AF_INET6)
				list_add_tail(parser_stringlist(item), &my_config->route_network6);
			else
				log_err("node [%s]: not ipv6 for route_network6='%s'", my_config->name, item);
			addrmask_free(addr);
		} else {
			log_err("node [%s]: received invalid route_network6='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "hidden="))) {
		parser_replace_item(&my_config->hidden, item);
	} else if ((item = parser_check_configitem(line, "silent="))) {
		parser_replace_item(&my_config->silent, item);
	} else if ((item = parser_check_configitem(line, "port="))) {
		parser_replace_item(&my_config->port, item);
	} else if ((item = parser_check_configitem(line, "indirectdata="))) {
		parser_replace_item(&my_config->indirectdata, item);
	} else if ((item = parser_check_configitem(line, "cipher="))) {
		parser_replace_item(&my_config->cipher, item);
	} else if ((item = parser_check_configitem(line, "compression="))) {
		parser_replace_item(&my_config->compression, item);
	} else if ((item = parser_check_configitem(line, "digest="))) {
		parser_replace_item(&my_config->digest, item);
	} else if ((item = parser_check_configitem(line, "primary="))) {
		parser_replace_item(&my_config->primary, item);
	} else if ((item = parser_check_configitem(line, "-----BEGIN RSA PUBLIC KEY-----"))) {
		free(my_config->key);
		my_config->key = calloc(sizeof(char), strlen(line) + 2);
		memcpy(my_config->key, line, strlen(line));
		strcat(my_config->key, "\n");
		parse_key_mode = true;
	} else if ((item = parser_check_configitem(line, "-----END RSA PUBLIC KEY-----"))) {
		parser_extend_key(line);
		parse_key_mode = false;
	} else {
		if (parse_key_mode) {
			parser_extend_key(line);
		} else {
			log_warn("parser: warning: unparsed: %s\n", line);
		}
	}
	return true;
}

bool
parser_parse_config (char *data, struct list_head *config_list)
{
	char *token;
	char *search = "\n";
	char *tmp;

	my_config = NULL;

	data = strdup(data);
	token = strtok_r(data, search, &tmp);
	while (token) {
		if (strncmp(token, "#", 1) &&
				!parser_parse_line(token, config_list))  {
			free(data);
			return false;
		}
		token = strtok_r(NULL, search, &tmp);
	}
	free(data);

	return true;
}

