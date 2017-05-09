#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <locale.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parser.h"
#include "list.h"
#include "extra.h"

#define VERSION "0.1.0"

extern int yylex();
extern int yyparse();

void yyerror(char *s) {
	fprintf(stderr, RED "ERROR: " RESET "%s\n", s);
}

static unsigned quickhash(char *sym) {
	register unsigned int hash = 0;
	register unsigned c;

	while ((c = *sym++))
		hash = hash*9 ^ c;

	return hash;
}

symbol *sym_add(symtab *st, char *sym) {
	symbol *sp = &st->head[quickhash(sym)%NHASH];
	int scount = NHASH;

	while (--scount >= 0) {
		if (sp->name && !strcmp(sp->name, sym)) {
            yyerror("symbol already defined");
            return NULL;
        }

		if (!sp->name) {
			sp->name = strndup(sym, 100);
			sp->v_ptr = NULL;
			st->count++;
			return sp;
		}

		if (++sp >= st->head+NHASH)
			sp = st->head;
	}
	
	yyerror("symbol table overflow");
	return NULL; //abort();
}

symbol *sym_lookup(symtab *st, char *sym) {
	symbol *sp;
	int scount = NHASH;

	do {
		sp = &st->head[quickhash(sym)%NHASH];
		while (--scount >= 0 && sp->name) {
        	if (!strcmp(sp->name, sym))
            	return sp;
        	if (++sp >= st->head+NHASH)
            	sp = st->head;
		}
		AH_PRINT("Symbol not found (in [%p]), checking parent...\n", st);
	} while ((st = st->parent));
    	AH_PRINT(YELLOW "symbol \"%s\" not found.\n" RESET, sym); // XXX
    yyerror("symbol not found");
    return NULL;
}

ast *new_ast(enum nodetype nodetype, ...) {
	ast *a = malloc(sizeof(ast));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = nodetype;

    va_list parameters;
    va_start(parameters, nodetype);
    a->children = &va_arg(parameters, ast*)->siblings;
     AH_PRINT(">>> %p\n", ast_left(a)->siblings.prev);
    if (a->nodetype != T_NEGATIVE && a->nodetype != T_NOT && a->nodetype != T_LIST) // XXX HACK fix this
        list_add_tail(&va_arg(parameters, ast*)->siblings, a->children);
     AH_PRINT(">>> %p\n", ast_left(a)->siblings.prev);

	AH_PRINT("\t(!) new ast %s [%p]\n", nodetype_to_string[nodetype], a);

	return a;
}

ast *new_ident(char *i) {
	node_identifier *a = malloc(sizeof(node_identifier));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

    a->nodetype = T_IDENTIFIER;
    a->i = i;
    AH_PRINT("\t(!) new identifier [%p]: %s\n", a, i);

    return (ast *)a;
}

ast *new_num(double d) {
	node_numval *a = malloc(sizeof(node_numval));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_NUMERIC;
	a->number = d;
	AH_PRINT("\t(!) new num [%p]: %f\n", a, d);

	return (ast *)a;
}

ast *new_bool(bool b) {
	node_boolval *a = malloc(sizeof(node_boolval));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_BOOLEAN;
	a->value = b;
	AH_PRINT("\t(!) new boolean [%p]: %s\n", a, b?"true":"false");

	return (ast *)a;
}

ast *new_asgn(char *i, ast *v) {
	node_assignment *a = malloc(sizeof(node_assignment));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_ASSIGNMENT;
	a->i = i;
	a->v = v;

	return (ast *)a;
}

ast *new_func(ast *args, ast *expr_list) {
	node_function *a = malloc(sizeof(node_function));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_FUNCTION;
    a->args = &args->siblings;
    a->children = &expr_list->siblings;

	AH_PRINT("\t(!) new func [%p]: args [%p] | expr_list [%p]\n", a, args, expr_list);

	return (ast *)a;
}

ast *new_env(ast *expr_list) {
	node_function *a = malloc(sizeof(node_function));
	INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_ENV;
    a->children = &expr_list->siblings;

	AH_PRINT("\t(!) new env [%p]: expr_list [%p]\n", a, expr_list);

	return (ast *)a;
}

ast *new_call(ast *func, ast *param_list) {
	ast *a = malloc(sizeof(ast));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_CALL;

    a->children = &new_ast(T_LIST, param_list)->siblings;
    list_add_tail(&func->siblings, a->children);

	AH_PRINT("\t(!) new call [%p]\n", a);

    /*
    AH_PRINT("=== PARAMETERS ===\n");
    ast *p;
    list_for_each_entry(p, &param_list->siblings, siblings) {
    	AH_NODE_INFO(p);
    }
    AH_PRINT("==================\n");
    */

	return a;
}

ast *new_if(ast *expr, ast *t, ast *f) {
	node_if *a = malloc(sizeof(node_if));
    INIT_LIST_HEAD(&a->siblings);

	if(!a) {
		yyerror("out of space");
		exit(0);
	}

	a->nodetype = T_IF;

    a->expr = expr;
    a->t = t;
    a->f = f;

	AH_PRINT("\t(!) new if [%p]\n", a);

	return (ast *)a;
}

void free_ast(ast **a) {
    *a = NULL; // TODO :)
}

symtab *new_symtab(symtab *parent) {
	symtab *st = malloc(sizeof(symtab));
	if (!st) {
		yyerror("unable to allocate more memory, exiting...");
		exit(-1);
	}

	st->head = calloc(NHASH, sizeof(symbol));
	if (!st->head) {
		yyerror("unable to allocate more memory, exiting...");
		exit(-1);
	}

	st->parent = parent;
	st->count = 0;

	return st;
}

void free_symtab(symtab *st) {
    AH_PRINT("Freeing symbol table: symtab [%p] | symtab.head [%p] | symtab.parent [%p] | symtab.count %i\n",
             st, st->head, st->parent, st->count);
    free(st->head); // XXX leaking identifier name
    free(st);
}

static inline void set_value(symbol *s, value *res) {
    if (s) {
    	switch (res->type) {
    		case BOOLEAN:
    		case NUMERIC:
    		case FUNCTION:
    		case CLOSURE:
    			s->v_ptr = malloc(sizeof(value));
    			memcpy(s->v_ptr, res, sizeof(value));
    			break;
    		default:
    			AH_PRINTX("Can't set value: unknown type\n");
				exit(-1);
    	}
    } else {
    	res->type = NOTHING;
    }
}

static inline void get_value(symbol *s, value *res) {
	if (s) {
        //res->type = s->v_ptr->type;
        //res->value = s->v_ptr->value;
        *res = *s->v_ptr;
    } else {
		res->type = NOTHING;
	}
}

// binary operations: numerical -> numerical -> numerical
void eval_n_n_n(ast *l, ast *r, symtab *st, value *res, enum nodetype op) {
    double n1, n2;

    eval(l, st, res);
    if (res->type == NOTHING) return;
    if (res->type != NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "LHS operand not numeric.\n");
		res->type = NOTHING;
        return;
    }
    n1 = res->value.n;

    eval(r, st, res);
    if (res->type == NOTHING) return;
    if (res->type != NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "RHS operand not numeric.\n");
		res->type = NOTHING;
        return;
    }
    n2 = res->value.n;

    switch (op) {
    	case T_SUM:
    		res->value.n = n1 + n2;
    		break;
    	case T_REST:
    		res->value.n = n1 - n2;
    		break;
    	case T_PRODUCT:
    		res->value.n = n1 * n2;
    		break;
    	case T_DIVISION:
    		res->value.n = n1 / n2;
    		break;
		case T_EXPONENTIATION:
			res->value.n = pow(n1, n2);
			break;
		case T_ROOT:
			res->value.n = pow(n2, 1/n1);
			break;
    	default:
			printf(RED "FATAL ERROR: invalid operator\n" RESET);
        	exit(-1);
    }
}

// binary operations: numerical -> numerical -> boolean
void eval_n_n_b(ast *l, ast *r, symtab *st, value *res, enum nodetype op) {
    double n1, n2;

    eval(l, st, res);
    if (res->type == NOTHING) return;
    if (res->type != NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "LHS operand not numeric.\n");
		res->type = NOTHING;
        return;
    }
    n1 = res->value.n;

    eval(r, st, res);
    if (res->type == NOTHING) return;
    if (res->type != NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "RHS operand not numeric.\n");
		res->type = NOTHING;
        return;
    }
    n2 = res->value.n;

    res->type = BOOLEAN;
    switch (op) {
		case T_EQUAL:
			res->value.b = n1 == n2;
			break;
		case T_NOTEQUAL:
			res->value.b = n1 != n2;
			break;
		case T_LESS:
			res->value.b = n1 < n2;
			break;
		case T_LESSEQUAL:
			res->value.b = n1 <= n2;
			break;
		case T_GREATER:
			res->value.b = n1 > n2;
			break;
		case T_GREATEREQUAL:
			res->value.b = n1 >= n2;
			break;
    	default:
			printf(RED "FATAL ERROR: invalid operator\n" RESET);
        	exit(-1);
    }
}

// binary operations: boolean -> boolean -> boolean
void eval_b_b_b(ast *l, ast *r, symtab *st, value *res, enum nodetype op) {
    double b1, b2;

    eval(l, st, res);
    if (res->type == NOTHING) return;
    if (res->type != BOOLEAN) {
        printf(RED "TYPE ERROR: " RESET "LHS operand not boolean.\n");
		res->type = NOTHING;
        return;
    }
    b1 = res->value.b;

    eval(r, st, res);
    if (res->type == NOTHING) return;
    if (res->type != BOOLEAN) {
        printf(RED "TYPE ERROR: " RESET "RHS operand not boolean.\n");
		res->type = NOTHING;
        return;
    }
    b2 = res->value.b;

    switch (op) {
		case T_AND:
			res->value.b = b1 && b2;
			break;
		case T_OR:
			res->value.b = b1 || b2;
			break;
    	default:
			printf(RED "FATAL ERROR: invalid operator\n" RESET);
        	exit(-1);
    }
}

void build_environment(symtab *parent_symtab, symtab *local_symtab, symtab *env, ast *a) {
	AH_NODE_INFO(a);
	ast *p;
	switch (a->nodetype) { // TODO use enum range macro to identify terminal and nonterminal (ie w/children) nodes
		case T_IDENTIFIER:
			if (!sym_lookup(local_symtab, ((node_identifier *)a)->i)) {
				// buscar en scopes arriba...
				AH_PRINT(CYAN ">>> FREE VARIABLE: %s\n" RESET, ((node_identifier *)a)->i);
				({
					value res;
					get_value(sym_lookup(parent_symtab, ((node_identifier *)a)->i), &res);
					if (res.type == NOTHING) {
						printf(RED "Free variable \"%s\" was not previously declared.\n" RESET, ((node_identifier *)a)->i);
					} else {
						AH_PRINT("Variable's scope found, adding to environment [%p].\n", env);
						set_value(sym_add(env, ((node_identifier *)a)->i), &res); // copio value
					}
				});
			} else {
				// nada para hacer
				AH_PRINT(">>> BINDED VARIABLE (nothing to do): %s\n", ((node_identifier *)a)->i);
			}
			break;
		case T_NUMERIC:
		case T_BOOLEAN:
			// Terminals
			break;
		case T_ASSIGNMENT:
			sym_add(local_symtab, ((node_assignment *)a)->i);
			break;
		case T_FUNCTION:
			({
				node_identifier *i;
				list_for_each_entry(i, ((node_function *)a)->args, siblings) {
					AH_PRINT(" args [%p]: %s\n", i, i->i);
					sym_add(local_symtab, i->i);
				}
			});
		case T_ENV:
			({
				symtab *inner_symtab = new_symtab(local_symtab); // XXX free?
				list_for_each_entry(p, a->children, siblings) {
					AH_PRINT("building --> %s\n", nodetype_to_string[p->nodetype]);
					build_environment(parent_symtab, inner_symtab, env, p);
				}
			});
			break;
		case T_CALL: // XXX check
			({
				puts(RED "PASSING THROUGH T_CALL..." RESET);//XXX remove

				node_identifier *i;
				list_for_each_entry(i, ast_left(a)->children, siblings) {
					AH_PRINT("~~~~~~~~~~~> args [%p]: %s\n", i, i->i);
					build_environment(parent_symtab, local_symtab, env, (ast *)i);
				}

				symtab *inner_symtab = new_symtab(local_symtab);
				AH_NODE_INFO(ast_right(a));
				build_environment(parent_symtab, inner_symtab, env, ast_right(a));
			});
			break;
		case T_LIST: // XXX check
		case T_SUM:
		case T_REST:
		case T_PRODUCT:
		case T_DIVISION:
		case T_EXPONENTIATION:
		case T_ROOT:
		case T_NEGATIVE:
		case T_EQUAL:
		case T_NOTEQUAL:
		case T_LESS:
		case T_LESSEQUAL:
		case T_GREATER:
		case T_GREATEREQUAL:
		case T_AND:
		case T_OR:
		case T_NOT:
			list_for_each_entry(p, a->children, siblings) {
				AH_PRINT("building --> %s\n", nodetype_to_string[p->nodetype]);
				build_environment(parent_symtab, local_symtab, env, p);
			}
			break;
		case T_IF:
			puts("building --> if_expr");
			build_environment(parent_symtab, local_symtab, env, ((node_if *)a)->expr);
			puts("building --> if_true");
			build_environment(parent_symtab, local_symtab, env, ((node_if *)a)->t);
			puts("building --> if_false");
			build_environment(parent_symtab, local_symtab, env, ((node_if *)a)->f);
			break;
		default:
			printf(RED "FATAL ERROR: %s: unknown value type: %i (%s)\n" RESET,
			       __func__,
			       a->nodetype,
			       nodetype_to_string[a->nodetype]);
			exit(-1);
	}
}

symtab *check_free_variables(symtab *parent_symtab, const node_function *const fun) {
    AH_PRINT(BLUE "=== SEARCHING FREE VARIABLES ===\n" RESET);

	symtab *tmp_symtab = new_symtab(NULL); // XXX To avoid lookup on outer scopes
	symtab *env = new_symtab(NULL); // XXX To avoid lookup on outer scopes

    node_identifier *i;
    list_for_each_entry(i, fun->args, siblings) {
        AH_PRINT(" args [%p]: %s\n", i, i->i);
		sym_add(tmp_symtab, i->i);
    }

    ast *p;
    list_for_each_entry(p, ((node_function *)fun)->children, siblings) {
        AH_PRINT(" expr_list [%p] nodetype: %s\n", p, nodetype_to_string[p->nodetype]);
        build_environment(parent_symtab, tmp_symtab, env, p);
    }

    free_symtab(tmp_symtab);
    if (!env->count) {
    	AH_PRINT("Temp environment unused, freeing...\n");
    	free_symtab(env);
    	env = NULL;
    }

    AH_PRINT(BLUE "================================\n" RESET);

    return env;
}

void eval(ast *a, symtab *st, value *res) {
    AH_PRINT(GREEN "EVAL: " RESET); AH_NODE_INFO(a);

	if (a) {
		switch (a->nodetype) {
			case T_NUMERIC:
                res->type = NUMERIC;
				res->value.n = ((node_numval*)a)->number;
				break;
			case T_BOOLEAN:
                res->type = BOOLEAN;
				res->value.b = ((node_boolval*)a)->value;
				break;
			case T_NEGATIVE:
				eval(ast_first_child(a), st, res);
    			if (res->type == NOTHING) return;
                if (res->type == NUMERIC)
					res->value.n *= -1;
				break;
			case T_SUM:
			case T_REST:
			case T_PRODUCT:
			case T_DIVISION:
			case T_EXPONENTIATION:
			case T_ROOT:
                eval_n_n_n(ast_left(a), ast_right(a), st, res, a->nodetype);
				break;
			case T_EQUAL:
			case T_NOTEQUAL:
			case T_LESS:
			case T_LESSEQUAL:
			case T_GREATER:
			case T_GREATEREQUAL:
                eval_n_n_b(ast_left(a), ast_right(a), st, res, a->nodetype);
				break;
			case T_AND:
			case T_OR:
                eval_b_b_b(ast_left(a), ast_right(a), st, res, a->nodetype);
				break;
			case T_NOT:
				eval(ast_first_child(a), st, res);
    			if (res->type == NOTHING) return;
				if (res->type != BOOLEAN) {
        			printf(RED "TYPE ERROR: " RESET "operand not boolean.\n");
					res->type = NOTHING;
				} else {
					res->value.b = !res->value.b;
				}
				break;
			case T_IF:
				({
    				eval(((node_if*)a)->expr, st, res);
    				if (res->type == NOTHING) return;
    				if (res->type != BOOLEAN) {
                        yyerror("Invalid type for if expression.");
        				res->type = NOTHING;
    				} else {
						if (res->value.b) {
							eval(((node_if*)a)->t, st, res);
						} else {
							eval(((node_if*)a)->f, st, res);
						}
					}
				});
				break;
			case T_ASSIGNMENT:
				eval(((node_assignment *)a)->v, st, res);
    			if (res->type == NOTHING) return;
                set_value(sym_add(st, ((node_assignment *)a)->i), res);
				break;
			case T_IDENTIFIER:
                AH_PRINTX(">>>>>> %s\n", ((node_identifier *)a)->i);
                get_value(sym_lookup(st, ((node_identifier *)a)->i), res);
				break;
            case T_ENV:
				({
					symtab *local_symtab = new_symtab(st);
                	ast *p;
                	list_for_each_entry(p, a->children, siblings) {
                    	eval(p, local_symtab, res);
                	}
                	free_symtab(local_symtab);
                });
            	break;
            case T_FUNCTION:
                ({
                    ast *p;
                    list_for_each_entry(p, ((node_function *)a)->args, siblings) {
                        AH_PRINT(" args [%p]: %s\n", p, ((node_identifier*)p)->i);
                    }
                    list_for_each_entry(p, ((node_function *)a)->children, siblings) {
                        AH_PRINT(" expr_list [%p] nodetype: %s\n", p, nodetype_to_string[p->nodetype]);
                    }
                });
                res->type = FUNCTION;
                res->value.f = a;
                break;
            case T_CALL:
                ({
                    ast *param_list = ast_left(a);
                    eval(ast_right(a), st, res);
    				if (res->type == NOTHING) return;

                    node_function *fun;
					symtab *local_symtab;

                    switch (res->type) {
                    	case FUNCTION:
 							fun = (node_function*)res->value.f;
							local_symtab = new_symtab(st);
							break;
						case CLOSURE:
 							fun = (node_function*)res->value.c.fun;
							local_symtab = new_symtab(res->value.c.env);
							local_symtab->parent->parent = st;
							break;
						default:
        					printf(RED "TYPE ERROR: " RESET "LHS operand is not a function.\n");
							res->type = NOTHING;
							return;
					}

                    ast *p;
                    list_for_each_entry(p, param_list->children, siblings) {
                        AH_PRINT(" p [%p] | %s | next:%p\n", p, nodetype_to_string[p->nodetype], p->siblings.next);
                    }

                    AH_PRINT("Assigning params to fun [%p]...\n", fun);
                    node_identifier *i;
                    p = list_entry(param_list->children, ast, siblings);
                    list_for_each_entry(i, fun->args, siblings) {
                        AH_PRINT(" PARAM: " CYAN "%s" RESET " [%p]: eval...\n", ((node_identifier*)i)->i, i);
						eval((ast *)p, st, res);
                        if (res->type == NOTHING) {
                        	yyerror("Invalid parameter");
                        	goto inv_param_exit; // XXX
                        }
                        AH_PRINT(" PARAM: " GREEN "%s" RESET " [%p]: adding to %p\n", ((node_identifier*)i)->i, i, local_symtab);
                        set_value(sym_add(local_symtab, ((node_identifier*)i)->i), res);
                        get_value(sym_lookup(local_symtab, ((node_identifier*)i)->i), res);
                        p = list_next_entry(p,siblings);
                    }
                    list_for_each_entry(p, fun->children, siblings) {
                    	eval((ast *)p, local_symtab, res);
                        // XXX do this just for the last p (ie the returned value)
                        AH_PRINT("p> [%p] nodetype: %s\n", p, nodetype_to_string[p->nodetype]);
                        if (res->type == FUNCTION) {
							symtab *env = check_free_variables(local_symtab, (node_function *) res->value.f);
							if (env) {
								puts("ENV HAVE SYMBOLS, RETURNING CLOSURE");
								res->type = CLOSURE;
								res->value.c.fun = (node_function *)res->value.f;
								res->value.c.env = env;
							} else {
								puts("ENV IS EMPTY, RETURNING FUNCTION");
							}
						}
                    }
                    inv_param_exit:
                    	free_symtab(local_symtab);
                });
                break;
            default:
                yyerror("unknown nodetype");
                // TODO deberia cambiar res?
		}
	} else {
		printf(RED "FATAL ERROR: eval null\n" RESET);
        exit(-1);
	}
}

int main(int argc, char **argv) {
    char* input;
    char prompt[] = "\001" MAGENTA "\002" "\xCE\xBB " "\001" RESET "\002";

    setlocale(LC_ALL, "");

    rl_readline_name = "ahci";
    rl_bind_key('\t', rl_abort);

    ast *a = NULL;

    NIL = malloc(sizeof(a));
    INIT_LIST_HEAD(&NIL->siblings);
    NIL->nodetype = T_NIL;

	symtab *global_symtab = new_symtab(NULL);

	printf("Welcome to Ad-hoc v%s.\n", VERSION);

    while ((input = readline(prompt))) {
        if (*input)
            add_history(input);
        if (parse_string(input, &a))
        	continue;

        ast *p;
        value res;
        list_for_each_entry(p, &a->siblings, siblings) {
            eval(p, global_symtab, &res);
            switch (res.type) {
            	case BOOLEAN:
            		printf("==> %s\n", res.value.b ? "true" : "false");
            		break;
            	case NUMERIC:
            		printf("==> %f\n", res.value.n);
            		break;
            	case FUNCTION:
            		printf("==> function [%p]\n", res.value.f);
            		break;
            	case CLOSURE:
            		printf("==> closure [%p]\n", res.value.c.fun);
            		break;
            	case NOTHING:
            		break;
            	default:
            		AH_PRINTX("unknown value type\n");
            }
        }

        //free_ast(&a);

        free(input);
    }

    puts("good-bye :-)");

	return 0;
}

