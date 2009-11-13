%{
#include "chaosvpn.h"
#include "y.tab.h"
%}
%Start PV SS
%%
\$my_peerid   {yylval.pval = &s_my_peerid; return KEYWORD_S;}
\$my_vpn_ip   {yylval.pval = &s_my_vpn_ip; return KEYWORD_S;}
\$my_vpn_ip6   {yylval.pval = &s_my_vpn_ip6; return KEYWORD_S;}
\$my_password   {yylval.pval = &s_my_password; return KEYWORD_S;}
\$my_ip   {yylval.pval = &s_my_ip; return KEYWORD_S;}
\$networkname   {yylval.pval = &s_networkname; return KEYWORD_S;}
\$tincd_bin   {yylval.pval = &s_tincd_bin; return KEYWORD_S;}
\$ip_bin   {yylval.pval = &s_ip_bin; return KEYWORD_S;}
\$ifconfig   {yylval.pval = &s_ifconfig; return KEYWORD_S;}
\$ifconfig6   {yylval.pval = &s_ifconfig6; return KEYWORD_S;}
\$master_url   {yylval.pval = &s_master_url; return KEYWORD_S;}
\$base   {yylval.pval = &s_base; return KEYWORD_S;}
\$pidfile   {yylval.pval = &s_pidfile; return KEYWORD_S;}
\$tincd_debuglevel   {yylval.pval = &s_tincd_debuglevel; return KEYWORD_I;}
\$my_vpn_netmask	{yylval.pval = &s_my_vpn_netmask; return KEYWORD_S;}

@exclude.*;

[ \t]*=[ \t]*	{ BEGIN PV; return ASSIGNMENT; }
<PV>[0-9]*\.[0-9]+	{yylval.fval = atof(yytext); return FLOATVAL;}
<PV>[0-9]+	{yylval.ival = atoi(yytext); return INTVAL;}
<PV>\"	{ BEGIN SS; return STRINGMARKER; }
<SS>\"	{ BEGIN 0; return STRINGMARKER; }
<SS>\\\$ {yylval.sval = strdup("$"); return STRING;}
<SS>[^"\\\$\n]* {yylval.sval = strdup(yytext); return STRING;}
;	{ return SEPARATOR; }
#.*
[ \t\n]*
