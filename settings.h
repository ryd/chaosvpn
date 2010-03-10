#ifndef __SETTINGS_H
#define __SETTINGS_H

/* this file should only be included from config.c and the lex/yacc code */

#include "list.h"
#include "config.h"


extern char* s_my_peerid;
extern char* s_my_vpn_ip;
extern char* s_my_vpn_ip6;
extern char* s_my_password;
extern char* s_my_ip;
extern char* s_networkname;
extern char* s_tincd_bin;
extern char* s_routeadd;
extern char* s_routeadd6;
extern char* s_routedel;
extern char* s_routedel6;
extern char* s_routemetric;
extern char* s_ifconfig;
extern char* s_ifconfig6;
extern char* s_master_url;
extern char* s_base;
extern char* s_masterdata_signkey;
extern char* s_pidfile;
extern char* s_interface;
extern char* s_my_vpn_netmask;
extern char* s_tincd_graphdumpfile;
extern char* s_tmpconffile;
extern unsigned int s_tincd_debuglevel;
extern unsigned int s_tincd_restart_delay;
extern unsigned int s_update_interval;
extern struct settings_list* s_exclude;

extern int yyparse(void);
extern void settings_init_defaults(void);
extern void settings_free_all(void);

#endif
