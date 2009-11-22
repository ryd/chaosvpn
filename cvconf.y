%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

extern int yyerror(char*);
extern int yylex (void);

extern char* catandfree(char*, char*, int, int);
extern char* concatias(int, char*, int);

extern int yycurline;
%}
%union {
    int ival;
    float fval;
    char* sval;
    void* pval;
}
%token <pval> KEYWORD_S
%token <pval> KEYWORD_I
%token <pval> KEYWORD_F
%token <sval> STRING
%token <ival> INTVAL
%token <fval> FLOATVAL
%token <sval> ASSIGNMENT
%token <sval> SEPARATOR
%token <sval> STRINGMARKER

%type <sval> string

%%
parser: { settings_init_defaults(); } config

config:
    | setting SEPARATOR config
    ;

setting: KEYWORD_I ASSIGNMENT INTVAL	{ *((int*)$1) = $3; }
    | KEYWORD_F ASSIGNMENT FLOATVAL	{ *((float*)$1) = $3; }
    | KEYWORD_S ASSIGNMENT STRINGMARKER string	STRINGMARKER	{ if (*(char**)$1) {free(*(char**)$1);} *((char**)$1) = $4; }
    ;

string: { $$ = strdup(""); /* ugly */ }
    | KEYWORD_S string { 
        if (*(char**)$1 == NULL) {
            fprintf(stderr, "error: access to uninitialized configuration variable in line %d\n", yycurline);
            exit(1);
        }
        $$ = catandfree(*(char**)$1, $2, 0, 1); 
    };
    | KEYWORD_I string { $$ = concatias(*(int*)$1, $2, 1); };
    | STRING string { $$ = catandfree($1, $2, 1, 1); }
    ;
%%


int
yyerror(char* msg)
{
    fprintf(stderr, "parse error: %s in line %d\n", msg, yycurline);
    return 0;
}

char*
concatias(int i, char* s, int frees)
{
    char* buf;
    size_t sl;
    size_t tl;

    sl = strlen(s);
    tl = sl + 20;
    buf = malloc(sl + tl);
    if (!buf) exit(111);
    snprintf(buf, sl + tl, "%d%s", i, s);
    if (frees) free(s);
    return buf;
}

char*
catandfree(char* s1, char* s2, int frees1, int frees2)
{
    size_t newlen;
    size_t l1, l2;
    char* buf;

    l1 = strlen(s1);
    l2 = strlen(s2);
    newlen = l1 + l2 + 1;
    buf = malloc(newlen);
    if(!buf) exit(111);

    memcpy(buf, s1, l1);
    memcpy(buf + l1, s2, l2);
    buf[newlen - 1] = 0;

    if (frees1) free(s1);
    if (frees2) free(s2);
    return buf;
}
