%option yylineno
%{
#include <stdio.h>
#include <ctype.h>

#include "parser.h"
#include "extra.h"
#include "grammar.tab.h"
%}

%%

";"         { return ';'; }
","         { return ','; }
":"         { return ':'; }
"("         { return '('; }
")"         { return ')'; }
 /* lambda abstraction term */
"["         { return '['; }
"]"         { return ']'; }
"|"         { return '|'; }
"{"         { return '{'; }
"}"         { return '}'; }
"@"         { return '@'; }
 /* mathematical operators */
"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }
"**"        { return EXP; }
"//"        { return ROOT; }
"log"       { return LOG; }
"mod"       { return MOD; }
"rem"       { return REM; }
"="         { return '='; }
"/="        { return NE; }
"<"         { return '<'; }
"<="        { return LE; }
">"         { return '>'; }
">="        { return GE; }
"and"       { return AND; }
"or"        { return OR; }
"not"       { return NOT; }
 /* lazy evaluation */
"$"         { return DELAY; }
"!"         { return FORCE; }
 /* branching */
"if"        { return IF; }
"cond"      { return COND; }
"case"      { return CASE; }
 /* higher-order functions */
"map"       { return MAP; }
"foldl"     { return FOLDL; }
"foldr"     { return FOLDR; }
"scanl"     { return SCANL; }
"scanr"     { return SCANR; }
"filter"    { return FILTER; }
 /* lists */
"head"      { return HEAD; }
"tail"      { return TAIL; }
"reverse"   { return REVERSE; }
"append"    { return APPEND; }
"length"    { return LENGTH; }
 /* literals */
"true"                  { return TRUE; }
"false"                 { return FALSE; }
[a-zA-Z_][a-zA-Z_0-9]*  { yylval.s = strdup(yytext); return IDENT; }
[0-9]+(\.[0-9]+)?       { yylval.d = atof(yytext); return NUMBER; }

 /* whitespace */
[ \r\t]     { }
\n          { return EOL; }

<<EOF>>     { return 0; }

%%

int yywrap(void) {
	AH_PRINT("EOF\n");
	return 1;
}

int parse_string(char *s, ast **a) {
	YY_BUFFER_STATE bp = yy_scan_string(s);
	yy_switch_to_buffer(bp);
	int error_code = yyparse(a);
	yy_delete_buffer(bp);
	return error_code;
}

