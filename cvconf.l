%{

#include "chaosvpn.h"
#include "y.tab.h"

extern struct config *globalconfig; /* private from config.c */

static int stringcaller;
%}
%Start PV SS LL
%option noyywrap
%%
\$my_peerid   {yylval.pval = &globalconfig->peerid; return KEYWORD_S;}
\$my_vpn_ip   {yylval.pval = &globalconfig->vpn_ip; return KEYWORD_S;}
\$my_vpn_ip6   {yylval.pval = &globalconfig->vpn_ip6; return KEYWORD_S;}
\$my_password   {yylval.pval = &globalconfig->password; return KEYWORD_S;}
\$my_ip   {yylval.pval = &globalconfig->my_ip; return KEYWORD_S;}
\$networkname   {yylval.pval = &globalconfig->networkname; return KEYWORD_S;}
\$tincd_bin   {yylval.pval = &globalconfig->tincd_bin; return KEYWORD_S;}
\$routeadd   {yylval.pval = &globalconfig->routeadd; return KEYWORD_S;}
\$routeadd6   {yylval.pval = &globalconfig->routeadd6; return KEYWORD_S;}
\$routedel   {yylval.pval = &globalconfig->routedel; return KEYWORD_S;}
\$routedel6   {yylval.pval = &globalconfig->routedel6; return KEYWORD_S;}
\$routemetric   {yylval.pval = &globalconfig->routemetric; return KEYWORD_S;}
\$ifconfig   {yylval.pval = &globalconfig->ifconfig; return KEYWORD_S;}
\$ifconfig6   {yylval.pval = &globalconfig->ifconfig6; return KEYWORD_S;}
\$master_url   {yylval.pval = &globalconfig->master_url; return KEYWORD_S;}
\$masterdata_signkey   {yylval.pval = &globalconfig->masterdata_signkey; return KEYWORD_S;}
\$base   {yylval.pval = &globalconfig->base_path; return KEYWORD_S;}
\$pidfile   {yylval.pval = &globalconfig->pidfile; return KEYWORD_S;}
\$tincd_debuglevel   {yylval.pval = &globalconfig->tincd_debuglevel; return KEYWORD_I;}
\$my_vpn_netmask	{yylval.pval = &globalconfig->vpn_netmask; return KEYWORD_S;}
\$tmpconffile	{yylval.pval = &globalconfig->tmpconffile; return KEYWORD_S;}
\$tincd_restart_delay   {yylval.pval = &globalconfig->tincd_restart_delay; return KEYWORD_I;}
\$tincd_graphdumpfile	{yylval.pval = &globalconfig->tincd_graphdumpfile; return KEYWORD_S;}
\$tincd_interface	{yylval.pval = &globalconfig->tincd_interface; return KEYWORD_S;}
\$tincd_device	{yylval.pval = &globalconfig->tincd_device; return KEYWORD_S;}
\$tincd_user	{yylval.pval = &globalconfig->tincd_user; return KEYWORD_S;}
\$update_interval {yylval.pval = &globalconfig->update_interval; return KEYWORD_I;}
\$use_dynamic_routes {yylval.pval = &globalconfig->use_dynamic_routes; return KEYWORD_B;}
\$connect_only_to_primary_nodes {yylval.pval = &globalconfig->connect_only_to_primary_nodes; return KEYWORD_B;}
@exclude {yylval.pval = &globalconfig->exclude; return KEYWORD_L;}
\[@$][_a-z]+   {
    log_err("parse error: unknown configuration variable "
            "\"%s\" in line %d\n", yytext, yylineno);
    exit(1);
}

[ \t]*=[ \t]*	{ BEGIN PV; return ASSIGNMENT; }
<PV>[0-9]*\.[0-9]+	{yylval.fval = atof(yytext); return FLOATVAL;}
<PV>[0-9]+	{yylval.ival = atoi(yytext); return INTVAL;}
<PV>\"	{ stringcaller = YY_START; BEGIN SS; return STRINGMARKER; }
<PV>\(  { BEGIN LL; return LISTOPEN; }
<SS>\"	{ BEGIN stringcaller; return STRINGMARKER; }
<SS>\\\$ {yylval.sval = strdup("$"); return STRING;}
<SS>[^"\\\$]* {yylval.sval = strdup(yytext); return STRING;}
<LL>[ \t]* {}
<LL>, { return LISTSEP; }
<LL>\"  { stringcaller = YY_START; BEGIN SS; return STRINGMARKER; }
<LL>[0-9]*\.[0-9]+	{yylval.fval = atof(yytext); return FLOATVAL;}
<LL>[0-9]+	{yylval.ival = atoi(yytext); return INTVAL;}
<LL>\)  { BEGIN 0; return LISTCLOSE; }
;	{ return SEPARATOR; }
#.*
[ \t]*
\n
