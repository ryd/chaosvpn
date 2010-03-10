#ifndef __PARSER_H
#define __PARSER_H

extern int parser_parse_config (char *data, struct list_head *config_list); 
extern void parser_free_config(struct list_head* configlist);

#endif
