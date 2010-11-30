%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chaosvpn.h"

extern struct config *globalconfig; /* private from config.c */

extern int yyerror(char*);
extern int yylex (void);

static char* catandfree(char*, char*, int, int);
static char* concatias(int, char*, int);
static struct settings_list_entry* list_mkientry(int);
static struct settings_list_entry* list_mksentry(const char const*);
static struct settings_list* list_add_elem(struct settings_list*, struct settings_list_entry*);
static struct settings_list* list_allocate(void);

extern int yylineno;
%}
%union {
    int ival;
    float fval;
    char* sval;
    void* pval;
    struct settings_list* lval;
    struct settings_list_entry* eval;
}
%token <pval> KEYWORD_S
%token <pval> KEYWORD_I
%token <pval> KEYWORD_F
%token <pval> KEYWORD_L
%token <pval> KEYWORD_B
%token <sval> STRING
%token <ival> INTVAL
%token <fval> FLOATVAL
%token <sval> ASSIGNMENT
%token <sval> SEPARATOR
%token <sval> STRINGMARKER
%token <pval> LISTOPEN
%token <pval> LISTSEP
%token <pval> LISTCLOSE

%type <sval> string
%type <lval> list
%type <eval> listentry

%%
parser: { } config

config:
    | setting SEPARATOR config
    ;

setting: KEYWORD_I ASSIGNMENT INTVAL	{ *((int*)$1) = $3; }
    | KEYWORD_B ASSIGNMENT INTVAL	{ *((bool*)$1) = $3; }
    | KEYWORD_F ASSIGNMENT FLOATVAL	{ *((float*)$1) = $3; }
    | KEYWORD_S ASSIGNMENT STRINGMARKER string STRINGMARKER	{ if (*(char**)$1) {free(*(char**)$1);} *((char**)$1) = $4; }
    | KEYWORD_L ASSIGNMENT LISTOPEN list { *((struct settings_list**)$1) = $4; }
    ;

listentry: INTVAL { $$ = list_mkientry($1); }
    | STRINGMARKER string STRINGMARKER { $$ = list_mksentry($2); }

list: listentry LISTCLOSE { $$ = list_add_elem(list_allocate(), $1); }
    | listentry LISTSEP list { $$ = list_add_elem($3, $1); }
    | LISTCLOSE { $$ = list_allocate(); }
    ;

string: { $$ = strdup(""); /* ugly */ }
    | KEYWORD_S string { 
        if (*(char**)$1 == NULL) {
            log_err("error: access to uninitialized configuration variable in line %d", yylineno);
            exit(1);
        }
        $$ = catandfree(*(char**)$1, $2, 0, 1); 
    };
    | KEYWORD_I string { $$ = concatias(*(int*)$1, $2, 1); };
    | STRING string { $$ = catandfree($1, $2, 1, 1); }
    ;
%%

static struct settings_list*
list_allocate(void)
{
    struct settings_list* sl = (struct settings_list*)
            malloc(sizeof(struct settings_list));
    if (sl == NULL) exit(111);
    INIT_LIST_HEAD(&sl->list);
    return sl;
}


static struct settings_list_entry*
list_mkientry(int i)
{
    struct settings_list_entry* entry;
    entry = malloc(sizeof(struct settings_list_entry));
    if (entry == NULL) exit(111);
    entry->etype = LIST_INTEGER;
    entry->evalue.i = i;
    return entry;
}

static struct settings_list_entry*
list_mksentry(const char const* s)
{
    struct settings_list_entry* entry;
    entry = malloc(sizeof(struct settings_list_entry));
    if (entry == NULL) exit(111);
    entry->etype = LIST_STRING;
    entry->evalue.s = strdup(s);
    return entry;
}

static struct settings_list*
list_add_elem(struct settings_list* list, struct settings_list_entry* item)
{
    struct settings_list* listitem;
    listitem = (struct settings_list*) malloc(sizeof(struct settings_list));
    if (listitem == NULL) exit(111);
    listitem->e = item;
    list_add(&listitem->list, &list->list);
    return list;
}

int
yyerror(char* msg)
{
    log_err("parse error: %s in line %d", msg, yylineno);
    exit(1);
}

static char*
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

static char*
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
