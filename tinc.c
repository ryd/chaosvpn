#include "main.h"
#include <stdio.h>

int tinc_generate_config(struct buffer *output, struct list_head *config_list) {
	char *config = "tincconfig";

	output->text = config;
	return 0;
}

