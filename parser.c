#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "main.h"
#include "parser.h"

struct config *my_config;
int parse_key_mode;

char *parser_check_new_item(char *token) {
	char *name = NULL;
	if (!strncmp(token, "[", 1) &&
			!strncmp(token + strlen(token) - 1, "]", 1)) {
		name = calloc(sizeof(char), strlen(token) - 1);
		memcpy(name, token + 1, strlen(token) - 2);
	}
	return name;
}

char *parser_check_configitem(char *line, char *config) {
	if (!strncmp(line, config, strlen(config))) {
		return line + strlen(config);
	}
	return NULL;
}

struct list_head *parser_stringlist (char *item) {
	struct stringlist *i = malloc(sizeof(struct stringlist));
	i->text = item;
	return &i->list;
}

void parser_extent_key(char *line) {
	my_config->key = realloc(
			my_config->key, 
			strlen(my_config->key) + strlen(line) + 2
	);
	strcat(my_config->key, line);
	strcat(my_config->key, "\n");
}

int parser_create_config(char *name){
	my_config = malloc(sizeof(struct config));

	INIT_LIST_HEAD(&my_config->network);
	INIT_LIST_HEAD(&my_config->network6);
	INIT_LIST_HEAD(&my_config->route_network);
	INIT_LIST_HEAD(&my_config->route_network6);

	my_config->name = name;
	my_config->gatewayhost = EMPTY;
	my_config->owner = EMPTY;
	my_config->use_tcp_only = EMPTY;
	my_config->hidden = EMPTY;
	my_config->silent = EMPTY;
	my_config->port = EMPTY;
	my_config->indirectdata = EMPTY;
	my_config->key = EMPTY;

	return 0;
}


int parser_parse_line(char *line, struct list_head *configlist) {
	char *item = parser_check_new_item(line);

	if (item) {
		struct configlist *i;

		parser_create_config(item);
		parse_key_mode = 0;

		i = malloc(sizeof *i);
		if (i == NULL) return 0;
		i->config = my_config;
		list_add_tail(&i->list, configlist);
	} else if ((item = parser_check_configitem(line, "gatewayhost="))) {
		my_config->gatewayhost = item;
	} else if ((item = parser_check_configitem(line, "owner="))) {
		my_config->owner = item;
	} else if ((item = parser_check_configitem(line, "use-tcp-only="))) {
		my_config->use_tcp_only = item;
	} else if ((item = parser_check_configitem(line, "network="))) {
		list_add_tail(parser_stringlist(item), &my_config->network);
	} else if ((item = parser_check_configitem(line, "network6="))) {
		list_add_tail(parser_stringlist(item), &my_config->network6);
	} else if ((item = parser_check_configitem(line, "route_network="))) {
		list_add_tail(parser_stringlist(item), &my_config->route_network);
	} else if ((item = parser_check_configitem(line, "route_network6="))) {
		list_add_tail(parser_stringlist(item), &my_config->route_network6);
	} else if ((item = parser_check_configitem(line, "hidden="))) {
		my_config->hidden = item;
	} else if ((item = parser_check_configitem(line, "silent="))) {
		my_config->silent = item;
	} else if ((item = parser_check_configitem(line, "port="))) {
		my_config->port = item;
	} else if ((item = parser_check_configitem(line, "indirectdata="))) {
		my_config->indirectdata = item;
	} else if ((item = parser_check_configitem(line, "-----BEGIN RSA PUBLIC KEY-----"))) {
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
	
	token = strtok(data, search);
	while (token) {
		if (strncmp(token, "#", 1) &&
				parser_parse_line(token, config_list)) 
			return 1;
		token = strtok(NULL, search);
	}

	return 0;
}

