#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "parser.h"
#include "list.h"
#include "extra.h"

extern int yylex();
extern int yyparse();

void yyerror(ast **a, char const *s) {
	fprintf(stderr, RED "ERROR: " RESET "%s\n", s);
}

ast *new_ast(node_type type, ...) {
	ast *a = malloc(sizeof(ast));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = type;

	va_list parameters;
	va_start(parameters, type);
	a->children = &va_arg(parameters, ast*)->siblings;
	 AH_PRINT(">>> %p\n", ast_left(a)->siblings.prev);
	if (a->type != NT_NEGATIVE && a->type != NT_NOT && a->type != NT_LIST) // XXX HACK fix this
		list_add_tail(&va_arg(parameters, ast*)->siblings, a->children);
	AH_PRINT(">>> %p\n", ast_left(a)->siblings.prev);

	AH_PRINT("\t(!) new ast %s [%p]\n", node_type_to_string[type], a);

	return a;
}

ast *new_ident(char *i) {
	node_identifier *a = malloc(sizeof(node_identifier));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_IDENTIFIER;
	a->i = i;
	AH_PRINT("\t(!) new identifier [%p]: %s\n", a, i);

	return (ast *)a;
}

ast *new_num(double d) {
	node_numval *a = malloc(sizeof(node_numval));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_NUMERIC;
	a->number = d;
	AH_PRINT("\t(!) new num [%p]: %f\n", a, d);

	return (ast *)a;
}

ast *new_bool(bool b) {
	node_boolval *a = malloc(sizeof(node_boolval));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_BOOLEAN;
	a->value = b;
	AH_PRINT("\t(!) new boolean [%p]: %s\n", a, b?"true":"false");

	return (ast *)a;
}

ast *new_asgn(char *i, ast *v) {
	node_assignment *a = malloc(sizeof(node_assignment));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_ASSIGNMENT;
	a->i = i;
	a->v = v;

	return (ast *)a;
}

ast *new_func(ast *args, ast *expr_list) {
	node_function *a = malloc(sizeof(node_function));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_FUNCTION;
	a->args = &args->siblings;
	a->children = &expr_list->siblings;

	AH_PRINT("\t(!) new func [%p]: args [%p] | expr_list [%p]\n", a, args, expr_list);

	return (ast *)a;
}

ast *new_env(ast *expr_list) {
	node_function *a = malloc(sizeof(node_function));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_ENV;
	a->children = &expr_list->siblings;

	AH_PRINT("\t(!) new env [%p]: expr_list [%p]\n", a, expr_list);

	return (ast *)a;
}

ast *new_call(ast *func, ast *param_list) {
	ast *a = malloc(sizeof(ast));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_APPLY;

	a->children = &new_ast(NT_LIST, param_list)->siblings;
	list_add_tail(&func->siblings, a->children);

	AH_PRINT("\t(!) new call [%p]\n", a);

	return a;
}

ast *new_if(ast *expr, ast *t, ast *f) {
	node_if *a = malloc(sizeof(node_if));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror(NULL, "out of space");
		exit(0);
	}

	a->type = NT_IF;

	a->expr = expr;
	a->t = t;
	a->f = f;

	AH_PRINT("\t(!) new if [%p]\n", a);

	return (ast *)a;
}

void free_ast(ast **a) {
	*a = NULL; // TODO :)
}

