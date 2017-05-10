#ifndef _AH_PARSER_H
#define _AH_PARSER_H

#include <stdbool.h>
#include "list.h"
#include "extra.h"

typedef enum {
    NOTHING,
    BOOLEAN,
    NUMERIC,
    FUNCTION,
    CLOSURE,
} value_type;

#define AH_NODETYPE(_) \
    _(T_NOTYPE) \
    _(T_NIL) \
    _(T_IDENTIFIER) \
    _(T_NUMERIC) \
    _(T_BOOLEAN) \
    _(T_LIST) \
    _(T_ASSIGNMENT) \
    _(T_FUNCTION) \
    _(T_ENV) \
    _(T_CALL) \
    _(T_ADDITION) \
    _(T_SUBTRACTION) \
    _(T_MULTIPLICATION) \
    _(T_DIVISION) \
    _(T_EXPONENTIATION) \
    _(T_ROOT) \
    _(T_NEGATIVE) \
    _(T_EQUAL) \
    _(T_NOTEQUAL) \
    _(T_LESS) \
    _(T_LESSEQUAL) \
    _(T_GREATER) \
    _(T_GREATEREQUAL) \
    _(T_AND) \
    _(T_OR) \
    _(T_NOT) \
    _(T_IF) \
    _(T__END) \

AH_DEFINE_ASSOCIATIVE_ENUM(nodetype, nodetype_to_string, AH_NODETYPE);

#define AH_NODE_INFO(n) \
    AH_PRINT("Node " BLUE #n RESET " [%p] is a " YELLOW "%s" RESET "\n", n, nodetype_to_string[n->nodetype]);

#define ast_first_child(pos) \
	list_entry((pos)->children, typeof(*(pos)), siblings)

#define ast_left(pos) \
	list_entry((pos)->children, typeof(*(pos)), siblings)

#define ast_right(pos) \
	list_entry((pos)->children->next, typeof(*(pos)), siblings)

typedef struct ast {
	enum nodetype nodetype;
    list_head siblings;
    list_head *children;
} ast;

ast *new_ast(enum nodetype nodetype, ...);
ast *new_ident(char*);
ast *new_num(double);
ast *new_bool(bool);
ast *new_asgn(char*, ast*);
ast *new_func(ast*, ast*);
ast *new_env(ast*);
ast *new_call(ast*, ast*);
ast *new_if(ast*, ast*, ast*);

void yyerror(ast**, char const*);
int parse_string(char*, ast**);

typedef struct symbol_ symbol;
typedef struct symtab_ symtab;
typedef struct node_function_ node_function;
typedef struct closure_ closure;

struct closure_ {
	symtab *env;
	node_function *fun;
};

typedef struct {
    value_type type;
    union {
        bool b;
        double n;
        ast *f;
        closure c;
    } value;
} value;

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

/* type T_NIL */
ast *NIL;

/* type T_IDENTIFIER */
typedef struct {
    enum nodetype nodetype;
    list_head siblings;
    char *i;
} node_identifier;

/* type T_NUMERIC */
typedef struct {
	enum nodetype nodetype;
    list_head siblings;
	double number;
} node_numval;

/* type T_BOOLEAN */
typedef struct {
	enum nodetype nodetype;
    list_head siblings;
	bool value;
} node_boolval;

/* type T_ASSIGNMENT */
typedef struct {
	enum nodetype nodetype;
    list_head siblings;
    list_head *children;
    char *i;
    ast *v;
} node_assignment;

/* type T_FUNCTION */
struct node_function_ {
	enum nodetype nodetype;
    list_head siblings;
    list_head *children;
    list_head *args;
};

/* type T_ENV */
typedef struct {
	enum nodetype nodetype;
    list_head siblings;
    list_head *children;
} node_env;

/* type T_IF */
typedef struct {
	enum nodetype nodetype;
    list_head siblings;
    ast *expr;
    ast *t;
    ast *f;
} node_if;

#endif

