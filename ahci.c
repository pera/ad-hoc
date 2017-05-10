#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <locale.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parser.h"
#include "symtab.h"

#define VERSION "0.1.0"

void eval_n_n_b(ast*, ast*, symtab*, value*, enum nodetype);
void eval_n_n_n(ast*, ast*, symtab*, value*, enum nodetype);
void eval_b_b_b(ast*, ast*, symtab*, value*, enum nodetype);
void eval(ast*, symtab*, value*);

void build_environment(symtab*, symtab*, symtab*, ast*);
symtab *check_free_variables(symtab*, const node_function *const);

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
    	case T_ADDITION:
    		res->value.n = n1 + n2;
    		break;
    	case T_SUBTRACTION:
    		res->value.n = n1 - n2;
    		break;
    	case T_MULTIPLICATION:
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
				AH_PRINT(RED "PASSING THROUGH T_CALL...\n" RESET);//XXX remove this

				ast *p;
				list_for_each_entry(p, ast_left(a)->children, siblings) {
					AH_PRINT("~~~~~~~~~~~> args [%p]: %s\n", p, nodetype_to_string[p->nodetype]);
					build_environment(parent_symtab, local_symtab, env, p);
				}

				symtab *inner_symtab = new_symtab(local_symtab);
				AH_NODE_INFO(ast_right(a));
				build_environment(parent_symtab, inner_symtab, env, ast_right(a));
			});
			break;
		case T_LIST: // XXX check
		case T_ADDITION:
		case T_SUBTRACTION:
		case T_MULTIPLICATION:
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
			AH_PRINT("building --> if_expr\n");
			build_environment(parent_symtab, local_symtab, env, ((node_if *)a)->expr);
			AH_PRINT("building --> if_true\n");
			build_environment(parent_symtab, local_symtab, env, ((node_if *)a)->t);
			AH_PRINT("building --> if_false\n");
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
			case T_ADDITION:
			case T_SUBTRACTION:
			case T_MULTIPLICATION:
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
                        yyerror(NULL, "Invalid type for if expression.");
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
                ({
					symbol *s = sym_lookup(st, ((node_identifier *)a)->i);
                	if (s) {
                		get_value(s, res);
                	} else {
    					yyerror(NULL, "symbol not found");
    					res->type = NOTHING;
    				}
    			});
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
                        	yyerror(NULL, "Invalid parameter");
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
								AH_PRINT("ENV HAVE SYMBOLS, RETURNING CLOSURE\n");
								res->type = CLOSURE;
								res->value.c.fun = (node_function *)res->value.f;
								res->value.c.env = env;
							} else {
								AH_PRINT("ENV IS EMPTY, RETURNING FUNCTION\n");
							}
						}
                    }
                    inv_param_exit:
                    	free_symtab(local_symtab);
                });
                break;
            default:
                yyerror(NULL, "unknown nodetype");
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

