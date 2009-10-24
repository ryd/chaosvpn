#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"

struct stringlist {
    struct list_head list;
    char *text;
};

struct config {
	char *name;
	char *gatewayhost;
	struct list_head network;
	struct list_head network6;
};

struct config my_config;

char *parser_check_new_item(char *token) {
	char *name = NULL;
	if (!strncmp(token, "[", 1) &&
			!strncmp(token + strlen(token) - 1, "]", 1)) {
		name = malloc(strlen(token));
		memcpy(name, token + 1, strlen(token) - 2);
		memcpy(name + strlen(token) - 2, "\0", 1); // I thing - there is a better solution
	}
	return name;
}

char *parser_check_configitem(char *line, char *config) {
		if (!strncmp(line, config, strlen(config))) {
			return line + strlen(config);
		}
		return NULL;
}

int parser_parse_line(char *line, struct config *config) {
	char *item = parser_check_new_item(line);
	if (item) {
		printf("new item:%s\n", item);
		config->name = item;
	} else if (item = parser_check_configitem(line, "gatewayhost=")) {
		//printf("config:gatwayhost %s\n", item);
	} else if (item = parser_check_configitem(line, "owner=")) {
		//printf("config:owner %s\n", item);
	} else if (item = parser_check_configitem(line, "use-tcp-only=")) {
		//printf("config:use-tcp-only %s\n", item);
	} else if (item = parser_check_configitem(line, "network=")) {
		//printf("config:network %s\n", item);
		struct stringlist *i = malloc(sizeof(struct stringlist));
		i->text = item;
		list_add_tail(&i->list, &config->network);
	} else if (item = parser_check_configitem(line, "network6=")) {
		//printf("config:network6 %s\n", item);
	} else if (item = parser_check_configitem(line, "route_network=")) {
		//printf("config:route_network %s\n", item);
	} else if (item = parser_check_configitem(line, "route_network6=")) {
		//printf("config:route_network6 %s\n", item);
	} else if (item = parser_check_configitem(line, "hidden=")) {
		//printf("config:hidden %s\n", item);
	} else if (item = parser_check_configitem(line, "silent=")) {
		//printf("config:silent %s\n", item);
	} else if (item = parser_check_configitem(line, "port=")) {
		//printf("config:port %s\n", item);
	} else if (item = parser_check_configitem(line, "indirectdata=")) {
		//printf("config:indirectdata %s\n", item);
	} else {
		printf("unparsed:%s\n", line);
	}
	return 0;
}

int parser_parse_config (char *data) {
	char *token;
	char *search = "\n";
	struct config *config_ptr = &my_config;
	INIT_LIST_HEAD(&config_ptr->network);

	token = strtok(data, search);
	while (token) {
		if (strncmp(token, "#", 1) &&
				parser_parse_line(token, config_ptr)) 
			return 1;
		token = strtok(NULL, search);
	}

	printf("last name:%s\n", config_ptr->name);
	struct list_head *p;
	list_for_each(p, &config_ptr->network) {
		struct stringlist *i = container_of(p, struct stringlist, list);
		printf("network:%s\n", i->text);
	}

	return 0;
}

