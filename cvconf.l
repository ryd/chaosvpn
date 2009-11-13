%{
#include "chaosvpn.h"
#include "y.tab.h"
%}
%Start PV SS
%%
\$my_peerid   {yylval.pval = &s_my_peerid; return KEYWORD;}
\$my_vpn_ip   {yylval.pval = &s_my_vpn_ip; return KEYWORD;}
\$my_vpn_ip6   {yylval.pval = &s_my_vpn_ip6; return KEYWORD;}
\$my_password   {yylval.pval = &s_my_password; return KEYWORD;}
\$my_ip   {yylval.pval = &s_my_ip; return KEYWORD;}
\$networkname   {yylval.pval = &s_networkname; return KEYWORD;}
\$tincd_bin   {yylval.pval = &s_tincd_bin; return KEYWORD;}
\$ip_bin   {yylval.pval = &s_ip_bin; return KEYWORD;}
\$ifconfig   {yylval.pval = &s_ifconfig; return KEYWORD;}
\$ifconfig6   {yylval.pval = &s_ifconfig6; return KEYWORD;}
\$master_url   {yylval.pval = &s_master_url; return KEYWORD;}
\$base   {yylval.pval = &s_base; return KEYWORD;}
\$pidfile   {yylval.pval = &s_pidfile; return KEYWORD;}
\$tincd_debuglevel   {yylval.pval = &s_tincd_debuglevel; return KEYWORD;}

@exclude.*;

[ \t]*=[ \t]*	{ BEGIN PV; return ASSIGNMENT; }
<PV>[0-9]*\.[0-9]+	{yylval.fval = atof(yytext); return FLOATVAL;}
<PV>[0-9]+	{yylval.ival = atoi(yytext); return INTVAL;}
<PV>\"	{ BEGIN SS; return STRINGMARKER; }
<SS>\"	{ BEGIN 0; return STRINGMARKER; }
<SS>\\\$ {yylval.sval = strdup("\\$"); return STRING;}
<SS>[^"\\\$\n]* {yylval.sval = strdup(yytext); return STRING;}
;	{ return SEPARATOR; }
#.*
[ \t\n]*
