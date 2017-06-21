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

%type <a> prog expr_list maybe_expr expr lit env list params_high params_low args built_in

%token EOL
%token ASGN
%token EXP
%token ROOT
%token LOG
%token MOD
%token REM
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
%token FOLDL
%token FOLDR
%token SCANL
%token SCANR
%token FILTER
%token HEAD
%token TAIL
%token REVERSE
%token APPEND
%token LENGTH

%token <s> IDENT
%token <d> NUMBER
%token <b> TRUE
%token <b> FALSE

%precedence ASGN
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
| env
| list { $$ = new_list($1); }
| IDENT { $$ = new_ident($1); }
| IDENT ASGN expr { $$ = new_asgn($1, $3); }
| expr '+' expr { $$ = new_ast(NT_ADDITION, $1, $3); }
| expr '-' expr { $$ = new_ast(NT_SUBTRACTION, $1, $3); }
| expr '*' expr { $$ = new_ast(NT_MULTIPLICATION, $1, $3); }
| expr '/' expr { $$ = new_ast(NT_DIVISION, $1, $3); }
| expr EXP expr { $$ = new_ast(NT_EXPONENTIATION, $1, $3); }
| expr ROOT expr { $$ = new_ast(NT_ROOT, $1, $3); }
| '-' expr %prec NEG { $$ = new_ast(NT_NEGATIVE, $2); }
| expr '=' expr { $$ = new_ast(NT_EQUAL, $1, $3); }
| expr NE expr { $$ = new_ast(NT_NOTEQUAL, $1, $3); }
| expr '<' expr { $$ = new_ast(NT_LESS, $1, $3); }
| expr LE expr { $$ = new_ast(NT_LESSEQUAL, $1, $3); }
| expr '>' expr { $$ = new_ast(NT_GREATER, $1, $3); }
| expr GE expr { $$ = new_ast(NT_GREATEREQUAL, $1, $3); }
| expr AND expr { $$ = new_ast(NT_AND, $1, $3); }
| expr OR expr { $$ = new_ast(NT_OR, $1, $3); }
| NOT expr { $$ = new_ast(NT_NOT, $2); }
| '(' expr ')' { $$ = $2; }
| '[' args '|' expr_list ']' { $$ = new_func($2, $4); }
| expr list { $$ = new_apply($1, $2); }
| expr '@' params_high { $$ = new_apply($1, $3); }
| IF expr env env { $$ = new_if($2, $3, $4); }
| built_in
;

lit
: NUMBER { $$ = new_num($1); }
| TRUE { $$ = new_bool(true); }
| FALSE { $$ = new_bool(false); }
;

env
: '[' expr_list ']' { $$ = new_env($2); }
;

list
: '{' params_low '}' { $$ = $2; }
| '{' '}' { $$ = NULL; }
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

built_in
: MAP { $$ = new_builtin(NT_MAP); }
| FOLDL { $$ = new_builtin(NT_FOLDL); }
| FOLDR { $$ = new_builtin(NT_FOLDR); }
| SCANL { $$ = new_builtin(NT_SCANL); }
| SCANR { $$ = new_builtin(NT_SCANR); }
| FILTER { $$ = new_builtin(NT_FILTER); }
| HEAD { $$ = new_builtin(NT_HEAD); }
| TAIL { $$ = new_builtin(NT_TAIL); }
| REVERSE { $$ = new_builtin(NT_REVERSE); }
| APPEND { $$ = new_builtin(NT_APPEND); }
| LENGTH { $$ = new_builtin(NT_LENGTH); }
;

%%

