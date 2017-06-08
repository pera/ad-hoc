#ifndef _AH_PARSER_H
#define _AH_PARSER_H

#include <stdbool.h>
#include "list.h"
#include "extra.h"

#define AH_VALUE_TYPE(_) \
        _(VT_NOTHING) \
        _(VT_BOOLEAN) \
        _(VT_NUMERIC) \
        _(VT_FUNCTION) \
        _(VT_LIST) \
        _(VT__END) \

AH_DEFINE_ASSOCIATIVE_ENUM(value_type, value_type_to_string, AH_VALUE_TYPE);
typedef enum value_type value_type;

#define AH_FUNCTION_TYPE(_) \
        _(FT_NEW) \
        _(FT_MAP) \
        _(FT_FOLDL) \
        _(FT_FOLDR) \
        _(FT_SCANL) \
        _(FT_SCANR) \
        _(FT_FILTER) \
        _(FT_HEAD) \
        _(FT_TAIL) \
        _(FT_REVERSE) \
        _(FT_APPEND) \
        _(FT_LENGTH) \
        _(FT__END) \

AH_DEFINE_ASSOCIATIVE_ENUM(function_type, function_type_to_string, AH_FUNCTION_TYPE);
typedef enum function_type function_type;

#define AH_NODE_TYPE(_) \
        _(NT_NOTYPE) \
        _(NT_NIL) \
        _(NT_IDENTIFIER) \
        _(NT_NUMERIC) \
        _(NT_BOOLEAN) \
        _(NT_LIST) \
        _(NT_ASSIGNMENT) \
        _(NT_FUNCTION) \
        _(NT_ENV) \
        _(NT_APPLY) \
        _(NT_ADDITION) \
        _(NT_SUBTRACTION) \
        _(NT_MULTIPLICATION) \
        _(NT_DIVISION) \
        _(NT_EXPONENTIATION) \
        _(NT_ROOT) \
        _(NT_NEGATIVE) \
        _(NT_EQUAL) \
        _(NT_NOTEQUAL) \
        _(NT_LESS) \
        _(NT_LESSEQUAL) \
        _(NT_GREATER) \
        _(NT_GREATEREQUAL) \
        _(NT_AND) \
        _(NT_OR) \
        _(NT_NOT) \
        _(NT_IF) \
        _(NT_MAP) \
        _(NT_FOLDL) \
        _(NT_FOLDR) \
        _(NT_SCANL) \
        _(NT_SCANR) \
        _(NT_FILTER) \
        _(NT_HEAD) \
        _(NT_TAIL) \
        _(NT_REVERSE) \
        _(NT_APPEND) \
        _(NT_LENGTH) \
        _(NT__END) \

AH_DEFINE_ASSOCIATIVE_ENUM(node_type, node_type_to_string, AH_NODE_TYPE);
typedef enum node_type node_type;

#define AH_NODE_INFO(n) \
	AH_PRINT("Node " BLUE #n RESET " [%p] is a " YELLOW "%s" RESET "\n", n, node_type_to_string[n->type]);

#define ast_first_child(pos) \
	list_entry((pos)->children, typeof(*(pos)), siblings)

#define ast_left(pos) \
	list_entry((pos)->children, typeof(*(pos)), siblings)

#define ast_right(pos) \
	list_entry((pos)->children->next, typeof(*(pos)), siblings)

typedef struct ast {
	node_type type;
	list_head siblings;
	list_head *children;
} ast;

ast *new_ast(node_type type, ...);
ast *new_ident(char*);
ast *new_num(double);
ast *new_bool(bool);
ast *new_asgn(char*, ast*);
ast *new_func(ast*, ast*);
ast *new_env(ast*);
ast *new_list(ast*);
ast *new_apply(ast*, ast*);
ast *new_if(ast*, ast*, ast*);
ast *new_builtin(node_type);

void free_ast(ast**);

void yyerror(ast**, char const*);
int parse_string(char*, ast**);

typedef struct value_function_ value_function;
typedef struct value_list_ value_list;
typedef struct value_ value;
typedef struct symbol_ symbol;
typedef struct symtab_ symtab;
typedef struct node_function_ node_function;

struct value_function_ {
	function_type type;
	node_function *node;
	symtab *env;
};

struct value_list_ {
	value *element;
	list_head siblings;
};

struct value_ {
	value_type type;
	union {
		bool b;
		double n;
		value_function f;
		value_list *l; // TODO change
	} value;
};

#define NHASH 9973

struct symbol_ {
	char *name;
	value *v_ptr; // TODO improve
};

struct symtab_ {
	symbol *head;
	symtab *parent;
	unsigned int count;
};

/* type NT_NIL */
ast *NIL;

/* type NT_IDENTIFIER */
typedef struct {
	node_type type;
	list_head siblings;
	char *i;
} node_identifier;

/* type NT_NUMERIC */
typedef struct {
	node_type type;
	list_head siblings;
	double number;
} node_numval;

/* type NT_BOOLEAN */
typedef struct {
	node_type type;
	list_head siblings;
	bool value;
} node_boolval;

/* type NT_ASSIGNMENT */
typedef struct {
	node_type type;
	list_head siblings;
	list_head *children;
	char *i;
	ast *v;
} node_assignment;

/* type NT_FUNCTION */
struct node_function_ {
	node_type type;
	list_head siblings;
	list_head *children;
	list_head *args;
};

/* type NT_IF */
typedef struct {
	node_type type;
	list_head siblings;
	ast *expr;
	ast *t;
	ast *f;
} node_if;

#endif

