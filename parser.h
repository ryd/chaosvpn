#ifndef __PARSER_H
#define __PARSER_H

int parser_parse_config (char *data, struct list_head *config_list); 
void parser_free_config(struct list_head* configlist);

#endif
