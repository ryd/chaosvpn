#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "chaosvpn.h"

static struct peer_config *my_config = NULL;
static struct list_head done_unknown_warnings;
static bool parse_key_mode;

static char*
parser_check_configitem(char *line, char *config)
{
	int len;

	len = strlen(config);
	if (!strncasecmp(line, config, len)) {
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
	my_config->use_tcp_only = false;
	my_config->hidden = false;
	my_config->silent = false;
	my_config->port = TINC_DEFAULT_PORT;
	my_config->indirectdata = false;
	my_config->key = strdup("");
	my_config->ecdsapublickey = strdup("");
	my_config->cipher = strdup("");
	my_config->compression = strdup("");
	my_config->digest = strdup("");
	my_config->primary = false;

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
	free(config->key);
	free(config->ecdsapublickey);
	free(config->cipher);
	free(config->compression);
	free(config->digest);
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

static void
parser_warn_unknown(char *line)
{
	char *label;
	char *saveptr;
	struct list_head* ptr;
	struct string_list *si;

	if (line == NULL)
		return;

	label = strtok_r(line, "=", &saveptr);
	if (label == NULL)
		goto output;

	list_for_each(ptr, &done_unknown_warnings) {
		si = container_of(ptr, struct string_list, list);

		if (strcasecmp(label, si->text) == 0) {
			/* output already happened */
			return;
		}
	}

	/* remember output */
	list_add_tail(parser_stringlist(label), &done_unknown_warnings);

output:
	log_warn("parser: warning: unparsed and ignored: '%s' - maybe a newer chaosvpn version needed?\n", line);
}

static bool
parser_parse_line(char *line, struct list_head *configlist)
{
	int len;
	char *item;

	line = str_trim(line);
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
		my_config->use_tcp_only = str_is_true(item, false);
	} else if ((item = parser_check_configitem(line, "network="))) {
		if (addrmask_verify_subnet(item, AF_INET)) {
			list_add_tail(parser_stringlist(item), &my_config->network);
		} else {
			log_err("node [%s]: received invalid ipv4 network='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "network6="))) {
		if (addrmask_verify_subnet(item, AF_INET6)) {
			list_add_tail(parser_stringlist(item), &my_config->network6);
		} else {
			log_err("node [%s]: received invalid ipv6 network6='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "route_network="))) {
		if (addrmask_verify_subnet(item, AF_INET)) {
			list_add_tail(parser_stringlist(item), &my_config->route_network);
		} else {
			log_err("node [%s]: received invalid ipv4 route_network='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "route_network6="))) {
		if (addrmask_verify_subnet(item, AF_INET6)) {
			list_add_tail(parser_stringlist(item), &my_config->route_network6);
		} else {
			log_err("node [%s]: received invalid ipv6 route_network6='%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "hidden="))) {
		my_config->hidden = str_is_true(item, false);
	} else if ((item = parser_check_configitem(line, "silent="))) {
		my_config->silent = str_is_true(item, false);
	} else if ((item = parser_check_configitem(line, "port="))) {
		unsigned long int longport = strtoul(item, NULL, 0);
		if (longport != ULONG_MAX) {
			my_config->port = (unsigned short) longport;
		} else {
			log_err("node [%s]: received invalid port number '%s'", my_config->name, item);
		}
	} else if ((item = parser_check_configitem(line, "indirectdata="))) {
		my_config->indirectdata = str_is_true(item, false);
	} else if ((item = parser_check_configitem(line, "cipher="))) {
		parser_replace_item(&my_config->cipher, item);
	} else if ((item = parser_check_configitem(line, "compression="))) {
		parser_replace_item(&my_config->compression, item);
	} else if ((item = parser_check_configitem(line, "digest="))) {
		parser_replace_item(&my_config->digest, item);
	} else if ((item = parser_check_configitem(line, "primary="))) {
		my_config->primary = str_is_true(item, false);
	} else if ((item = parser_check_configitem(line, "ecdsapublickey="))) {
		parser_replace_item(&my_config->ecdsapublickey, item);
	} else if ((item = parser_check_configitem(line, "pingtest="))) {
		/* allow, but ignore in chaosvpn client */
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
			parser_warn_unknown(line);
		}
	}
	return true;
}

bool
parser_parse_config (char *data, struct list_head *config_list)
{
	char *token;
	char *search = "\n";
	char *saveptr;

	my_config = NULL;
	parse_key_mode = false;
	INIT_LIST_HEAD(&done_unknown_warnings);

	data = strdup(data);
	token = strtok_r(data, search, &saveptr);
	while (token) {
		if (strncmp(token, "#", 1) &&
				!parser_parse_line(token, config_list))  {
			free(data);
			parser_delete_string_list(&done_unknown_warnings);
			return false;
		}
		token = strtok_r(NULL, search, &saveptr);
	}
	free(data);

	parser_delete_string_list(&done_unknown_warnings);

	return true;
}

