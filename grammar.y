%{
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "parser.h"

extern int yylex();
/*
extern int yyerror(ast**, char const*); // XXX eliminar?
*/
%}
%error-verbose

%union {
	ast *a;
	char *s;
	double d;
	bool b;
}

%type <a> prog expr_list maybe_expr expr lit env params_high params_low args

%token EOL
%token EXP
%token ROOT
%token NE
%token LE
%token GE
%token AND
%token OR
%token NOT
%token DELAY
%token FORCE
%token IF
%token COND
%token CASE
%token MAP
%token FOLD
%token FILTER

%token <s> IDENT
%token <d> NUMBER
%token <b> TRUE
%token <b> FALSE

%precedence ':'
%left AND OR NOT
%left '=' NE
%left '<' LE '>' GE
%left '+' '-'
%left '*' '/'
%left EXP ROOT
%precedence NEG

%precedence '{'

%precedence PARAM_HIGH
%precedence ','
%precedence '@'

%parse-param { ast **root }

%start prog

%%

prog
: expr_list { *root = $1; }
;

expr_list
: maybe_expr
| expr_list ';' maybe_expr { if ($3 && $1) list_add_tail(&$3->siblings, &$1->siblings); }
;

maybe_expr
: %empty { $$ = NULL; }
| expr
;

expr
: lit
| IDENT { $$ = new_ident($1); }
| IDENT ':' expr { $$ = new_asgn($1, $3); }
| expr '+' expr { $$ = new_ast(T_SUM, $1, $3); }
| expr '-' expr { $$ = new_ast(T_REST, $1, $3); }
| expr '*' expr { $$ = new_ast(T_PRODUCT, $1, $3); }
| expr '/' expr { $$ = new_ast(T_DIVISION, $1, $3); }
| expr EXP expr { $$ = new_ast(T_EXPONENTIATION, $1, $3); }
| expr ROOT expr { $$ = new_ast(T_ROOT, $1, $3); }
| '-' expr %prec NEG { $$ = new_ast(T_NEGATIVE, $2); }
| expr '=' expr { $$ = new_ast(T_EQUAL, $1, $3); }
| expr NE expr { $$ = new_ast(T_NOTEQUAL, $1, $3); }
| expr '<' expr { $$ = new_ast(T_LESS, $1, $3); }
| expr LE expr { $$ = new_ast(T_LESSEQUAL, $1, $3); }
| expr '>' expr { $$ = new_ast(T_GREATER, $1, $3); }
| expr GE expr { $$ = new_ast(T_GREATEREQUAL, $1, $3); }
| expr AND expr { $$ = new_ast(T_AND, $1, $3); }
| expr OR expr { $$ = new_ast(T_OR, $1, $3); }
| NOT expr { $$ = new_ast(T_NOT, $2); }
| '(' expr ')' { $$ = $2; }
| env
| '[' args '|' expr_list ']' { $$ = new_func($2, $4); }
| expr '{' params_low '}' { $$ = new_call($1, $3); }
| expr '@' params_high { $$ = new_call($1, $3); }
| IF expr env env { $$ = new_if($2, $3, $4); }
;

lit
: NUMBER { $$ = new_num($1); }
| TRUE { $$ = new_bool(true); }
| FALSE { $$ = new_bool(false); }
;

env
: '[' expr_list ']' { $$ = new_env($2); }
;

params_low
: expr
| params_low ',' expr { if ($3 && $1) list_add_tail(&$3->siblings, &$1->siblings); }
;

params_high
: expr %prec PARAM_HIGH
| expr ',' params_high %prec PARAM_HIGH { if ($1 && $3) list_add_tail(&$1->siblings, &$3->siblings); }
;

args
: IDENT { $$ = new_ident($1); }
| args ',' IDENT { if ($1 && $3) list_add_tail(&new_ident($3)->siblings, &$1->siblings); }
;

%%

