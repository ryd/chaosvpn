#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "main.h"
#include "parser.h"

struct peer_config *my_config;
int parse_key_mode;

static char *parser_check_new_item(char *token) {
	char *name = NULL;
	if (!strncmp(token, "[", 1) &&
			!strncmp(token + strlen(token) - 1, "]", 1)) {
		name = calloc(sizeof(char), strlen(token) - 1);
		memcpy(name, token + 1, strlen(token) - 2);
	}
	return name;
}

static char *parser_check_configitem(char *line, char *config) {
	if (!strncmp(line, config, strlen(config))) {
		return line + strlen(config);
	}
	return NULL;
}

static struct list_head *parser_stringlist (char *item) {
	struct string_list *i = malloc(sizeof(struct string_list));
	i->text = item;
	return &i->list;
}

static void parser_extent_key(char *line) {
	my_config->key = realloc(
			my_config->key, 
			strlen(my_config->key) + strlen(line) + 2
	);
	strcat(my_config->key, line);
	strcat(my_config->key, "\n");
}

static int parser_create_config(char *name){
	my_config = malloc(sizeof(struct peer_config));
	memset(my_config, 0, sizeof(struct peer_config));

	INIT_LIST_HEAD(&my_config->network);
	INIT_LIST_HEAD(&my_config->network6);
	INIT_LIST_HEAD(&my_config->route_network);
	INIT_LIST_HEAD(&my_config->route_network6);

	my_config->name = name;
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

	return 0;
}

/*
TODO: not sure where to put this, to cleanup our memory handling we need
something like this soon

static void parser_free_config(struct peer_config *config) {
	if (!config) {
		// TODO: also free sublists if allocated
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
		free(config);
	}
}
*/

static void parser_replace_item(char **var, char *newitem) {
	free(*var);
	*var = newitem;
}

static int parser_parse_line(char *line, struct list_head *configlist) {
	char *item = parser_check_new_item(line);

	if (item) {
		struct peer_config_list *i;
		parse_key_mode = 0;

		i = malloc(sizeof *i);
		if (i == NULL) {
			free(my_config);
			return 0;
		}

		parser_create_config(item);
		i->peer_config = my_config;
		list_add_tail((struct list_head *)i, configlist);
	} else if (my_config) {
		/* we did not start with a [...] header */
		/* skip until after first valid header initialized a config section */
		return 0;
	} else if ((item = parser_check_configitem(line, "gatewayhost="))) {
		parser_replace_item(&my_config->gatewayhost, item);
	} else if ((item = parser_check_configitem(line, "owner="))) {
		parser_replace_item(&my_config->owner, item);
	} else if ((item = parser_check_configitem(line, "use-tcp-only="))) {
		parser_replace_item(&my_config->use_tcp_only, item);
	} else if ((item = parser_check_configitem(line, "network="))) {
		list_add_tail(parser_stringlist(item), &my_config->network);
	} else if ((item = parser_check_configitem(line, "network6="))) {
		list_add_tail(parser_stringlist(item), &my_config->network6);
	} else if ((item = parser_check_configitem(line, "route_network="))) {
		list_add_tail(parser_stringlist(item), &my_config->route_network);
	} else if ((item = parser_check_configitem(line, "route_network6="))) {
		list_add_tail(parser_stringlist(item), &my_config->route_network6);
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
	} else if ((item = parser_check_configitem(line, "-----BEGIN RSA PUBLIC KEY-----"))) {
		free(my_config->key);
		my_config->key = calloc(sizeof(char), strlen(line) + 2);
		memcpy(my_config->key, line, strlen(line));
		strcat(my_config->key, "\n");
		parse_key_mode = 1;
	} else if ((item = parser_check_configitem(line, "-----END RSA PUBLIC KEY-----"))) {
		parser_extent_key(line);
		parse_key_mode = 0;
	} else {
		if (parse_key_mode) {
			parser_extent_key(line);
		} else {
			printf("unparsed:%s\n", line);
		}
	}
	return 0;
}

int parser_parse_config (char *data, struct list_head *config_list) {
	char *token;
	char *search = "\n";

	my_config = NULL;

	token = strtok(data, search);
	while (token) {
		if (strncmp(token, "#", 1) &&
				parser_parse_line(token, config_list)) 
			return 1;
		token = strtok(NULL, search);
	}

	return 0;
}

