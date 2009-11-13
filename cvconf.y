%{
#include <stdio.h>
#include "chaosvpn.h"

extern int yyerror(char*);
extern int yylex (void);
%}
%union {
    int ival;
    float fval;
    char* sval;
    void* pval;
}
%token <pval> KEYWORD
%token <sval> STRING
%token <ival> INTVAL
%token <fval> FLOATVAL
%token <sval> ASSIGNMENT
%%
config: setting
    | setting config
    ;

setting: KEYWORD ASSIGNMENT STRING	{*((char**)$1) = $3;}
    | KEYWORD ASSIGNMENT INTVAL		{*((int*)$1) = $3;}
    | KEYWORD ASSIGNMENT FLOATVAL	{*((float*)$1) = $3;}
    ;
%%


int
yyerror(char* msg)
{
    fputs(msg, stderr);
    return 0;
}

