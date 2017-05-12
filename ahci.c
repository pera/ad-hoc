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

void eval_n_n_b(ast*, ast*, symtab*, value*, node_type);
void eval_n_n_n(ast*, ast*, symtab*, value*, node_type);
void eval_b_b_b(ast*, ast*, symtab*, value*, node_type);
void eval(ast*, symtab*, value*);

void build_environment(symtab*, symtab*, symtab*, ast*);
symtab *check_free_variables(symtab*, const node_function *const);

// binary operations: numerical -> numerical -> numerical
void eval_n_n_n(ast *l, ast *r, symtab *st, value *res, node_type op) {
    double n1, n2;

    eval(l, st, res);
    if (res->type == VT_NOTHING) return;
    if (res->type != VT_NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "LHS operand not numeric.\n");
		res->type = VT_NOTHING;
        return;
    }
    n1 = res->value.n;

    eval(r, st, res);
    if (res->type == VT_NOTHING) return;
    if (res->type != VT_NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "RHS operand not numeric.\n");
		res->type = VT_NOTHING;
        return;
    }
    n2 = res->value.n;

    switch (op) {
    	case NT_ADDITION:
    		res->value.n = n1 + n2;
    		break;
    	case NT_SUBTRACTION:
    		res->value.n = n1 - n2;
    		break;
    	case NT_MULTIPLICATION:
    		res->value.n = n1 * n2;
    		break;
    	case NT_DIVISION:
    		res->value.n = n1 / n2;
    		break;
		case NT_EXPONENTIATION:
			res->value.n = pow(n1, n2);
			break;
		case NT_ROOT:
			res->value.n = pow(n2, 1/n1);
			break;
    	default:
			printf(RED "FATAL ERROR: invalid operator\n" RESET);
        	exit(EXIT_FAILURE);
    }
}

// binary operations: numerical -> numerical -> boolean
void eval_n_n_b(ast *l, ast *r, symtab *st, value *res, node_type op) {
    double n1, n2;

    eval(l, st, res);
    if (res->type == VT_NOTHING) return;
    if (res->type != VT_NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "LHS operand not numeric.\n");
		res->type = VT_NOTHING;
        return;
    }
    n1 = res->value.n;

    eval(r, st, res);
    if (res->type == VT_NOTHING) return;
    if (res->type != VT_NUMERIC) {
        printf(RED "TYPE ERROR: " RESET "RHS operand not numeric.\n");
		res->type = VT_NOTHING;
        return;
    }
    n2 = res->value.n;

    res->type = VT_BOOLEAN;
    switch (op) {
		case NT_EQUAL:
			res->value.b = n1 == n2;
			break;
		case NT_NOTEQUAL:
			res->value.b = n1 != n2;
			break;
		case NT_LESS:
			res->value.b = n1 < n2;
			break;
		case NT_LESSEQUAL:
			res->value.b = n1 <= n2;
			break;
		case NT_GREATER:
			res->value.b = n1 > n2;
			break;
		case NT_GREATEREQUAL:
			res->value.b = n1 >= n2;
			break;
    	default:
			printf(RED "FATAL ERROR: invalid operator\n" RESET);
        	exit(EXIT_FAILURE);
    }
}

// binary operations: boolean -> boolean -> boolean
void eval_b_b_b(ast *l, ast *r, symtab *st, value *res, node_type op) {
    double b1, b2;

    eval(l, st, res);
    if (res->type == VT_NOTHING) return;
    if (res->type != VT_BOOLEAN) {
        printf(RED "TYPE ERROR: " RESET "LHS operand not boolean.\n");
		res->type = VT_NOTHING;
        return;
    }
    b1 = res->value.b;

    eval(r, st, res);
    if (res->type == VT_NOTHING) return;
    if (res->type != VT_BOOLEAN) {
        printf(RED "TYPE ERROR: " RESET "RHS operand not boolean.\n");
		res->type = VT_NOTHING;
        return;
    }
    b2 = res->value.b;

    switch (op) {
		case NT_AND:
			res->value.b = b1 && b2;
			break;
		case NT_OR:
			res->value.b = b1 || b2;
			break;
    	default:
			printf(RED "FATAL ERROR: invalid operator\n" RESET);
        	exit(EXIT_FAILURE);
    }
}

void build_environment(symtab *parent_symtab, symtab *local_symtab, symtab *env, ast *a) {
	AH_NODE_INFO(a);
	ast *p;
	switch (a->type) { // TODO use enum range macro to identify terminal and nonterminal (ie w/children) nodes
		case NT_IDENTIFIER: {
			if (!sym_lookup(local_symtab, ((node_identifier *)a)->i)) {
				AH_PRINT(CYAN ">>> FREE VARIABLE: %s\n" RESET, ((node_identifier *)a)->i);
				value res;
				get_value(sym_lookup(parent_symtab, ((node_identifier *)a)->i), &res);
				if (res.type == VT_NOTHING) {
					printf(RED "Free variable \"%s\" was not previously declared.\n" RESET, ((node_identifier *)a)->i);
				} else {
					AH_PRINT("Variable's scope found, adding to environment [%p].\n", env);
					if (res.type == VT_FUNCTION) {
						build_environment(parent_symtab, local_symtab, env, res.value.f);
					} 
					set_value(sym_add(env, ((node_identifier *)a)->i), &res);
				}
			} else {
				// nada para hacer
				AH_PRINT(">>> BINDED VARIABLE (nothing to do): %s\n", ((node_identifier *)a)->i);
			}
			break;
		}
		case NT_NUMERIC:
		case NT_BOOLEAN:
			// Terminals
			break;
		case NT_ASSIGNMENT:
			sym_add(local_symtab, ((node_assignment *)a)->i);
			break;
		case NT_FUNCTION: {
			node_identifier *i;
			list_for_each_entry(i, ((node_function *)a)->args, siblings) {
				AH_PRINT(" args [%p]: %s\n", i, i->i);
				sym_add(local_symtab, i->i);
			}
		}
		case NT_ENV: {
			symtab *inner_symtab = new_symtab(local_symtab); // XXX free?
			list_for_each_entry(p, a->children, siblings) {
				AH_PRINT("building --> %s\n", node_type_to_string[p->type]);
				build_environment(parent_symtab, inner_symtab, env, p);
			}
		}
			break;
		case NT_CALL: { // XXX check
			AH_PRINT(RED "PASSING THROUGH NT_CALL...\n" RESET);//XXX remove this

			ast *p;
			list_for_each_entry(p, ast_left(a)->children, siblings) {
				AH_PRINT("~~~~~~~~~~~> args [%p]: %s\n", p, node_type_to_string[p->type]);
				build_environment(parent_symtab, local_symtab, env, p);
			}

			symtab *inner_symtab = new_symtab(local_symtab);
			AH_NODE_INFO(ast_right(a));
			build_environment(parent_symtab, inner_symtab, env, ast_right(a));
			break;
		}
		case NT_LIST: // XXX check
		case NT_ADDITION:
		case NT_SUBTRACTION:
		case NT_MULTIPLICATION:
		case NT_DIVISION:
		case NT_EXPONENTIATION:
		case NT_ROOT:
		case NT_NEGATIVE:
		case NT_EQUAL:
		case NT_NOTEQUAL:
		case NT_LESS:
		case NT_LESSEQUAL:
		case NT_GREATER:
		case NT_GREATEREQUAL:
		case NT_AND:
		case NT_OR:
		case NT_NOT:
			list_for_each_entry(p, a->children, siblings) {
				AH_PRINT("building --> %s\n", node_type_to_string[p->type]);
				build_environment(parent_symtab, local_symtab, env, p);
			}
			break;
		case NT_IF:
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
			       a->type,
			       node_type_to_string[a->type]);
			exit(EXIT_FAILURE);
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
        AH_PRINT(" expr_list [%p] type: %s\n", p, node_type_to_string[p->type]);
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
		switch (a->type) {
			case NT_NUMERIC:
                res->type = VT_NUMERIC;
				res->value.n = ((node_numval*)a)->number;
				break;
			case NT_BOOLEAN:
                res->type = VT_BOOLEAN;
				res->value.b = ((node_boolval*)a)->value;
				break;
			case NT_NEGATIVE:
				eval(ast_first_child(a), st, res);
    			if (res->type == VT_NOTHING) return;
                if (res->type == VT_NUMERIC)
					res->value.n *= -1;
				break;
			case NT_ADDITION:
			case NT_SUBTRACTION:
			case NT_MULTIPLICATION:
			case NT_DIVISION:
			case NT_EXPONENTIATION:
			case NT_ROOT:
                eval_n_n_n(ast_left(a), ast_right(a), st, res, a->type);
				break;
			case NT_EQUAL:
			case NT_NOTEQUAL:
			case NT_LESS:
			case NT_LESSEQUAL:
			case NT_GREATER:
			case NT_GREATEREQUAL:
                eval_n_n_b(ast_left(a), ast_right(a), st, res, a->type);
				break;
			case NT_AND:
			case NT_OR:
                eval_b_b_b(ast_left(a), ast_right(a), st, res, a->type);
				break;
			case NT_NOT:
				eval(ast_first_child(a), st, res);
    			if (res->type == VT_NOTHING) return;
				if (res->type != VT_BOOLEAN) {
        			printf(RED "TYPE ERROR: " RESET "operand not boolean.\n");
					res->type = VT_NOTHING;
				} else {
					res->value.b = !res->value.b;
				}
				break;
			case NT_IF: {
    			eval(((node_if*)a)->expr, st, res);
    			if (res->type == VT_NOTHING) return;
    			if (res->type != VT_BOOLEAN) {
                    yyerror(NULL, "Invalid type for if expression.");
        			res->type = VT_NOTHING;
    			} else {
					if (res->value.b) {
						eval(((node_if*)a)->t, st, res);
					} else {
						eval(((node_if*)a)->f, st, res);
					}
				}
				break;
			}
			case NT_ASSIGNMENT:
				eval(((node_assignment *)a)->v, st, res);
    			if (res->type == VT_NOTHING) return;
                set_value(sym_add(st, ((node_assignment *)a)->i), res);
				break;
			case NT_IDENTIFIER: {
				symbol *s = sym_lookup(st, ((node_identifier *)a)->i);
                if (s) {
                	get_value(s, res);
                } else {
    				yyerror(NULL, "symbol not found");
    				res->type = VT_NOTHING;
    			}
				break;
    		}
            case NT_ENV: {
				symtab *local_symtab = new_symtab(st);
                ast *p;
                list_for_each_entry(p, a->children, siblings) {
                    eval(p, local_symtab, res);
                }
                free_symtab(local_symtab);
            	break;
            }
            case NT_FUNCTION: {
                ast *p;
                list_for_each_entry(p, ((node_function *)a)->args, siblings) {
                    AH_PRINT(" args [%p]: %s\n", p, ((node_identifier*)p)->i);
                }
                list_for_each_entry(p, ((node_function *)a)->children, siblings) {
                    AH_PRINT(" expr_list [%p] type: %s\n", p, node_type_to_string[p->type]);
                }
                res->type = VT_FUNCTION;
                res->value.f = a;
                break;
            }
            case NT_CALL: {
                ast *param_list = ast_left(a);
                eval(ast_right(a), st, res);
    			if (res->type == VT_NOTHING) return;

                node_function *fun;
				symtab *local_symtab;

                switch (res->type) {
                    case VT_FUNCTION:
 						fun = (node_function*)res->value.f;
						local_symtab = new_symtab(st);
						break;
					case VT_CLOSURE:
 						fun = (node_function*)res->value.c.fun;
						local_symtab = new_symtab(res->value.c.env);
						local_symtab->parent->parent = st;
						break;
					default:
        				printf(RED "TYPE ERROR: " RESET "LHS operand is not a function.\n");
						res->type = VT_NOTHING;
						return;
				}

                ast *p;
                list_for_each_entry(p, param_list->children, siblings) {
                    AH_PRINT(" p [%p] | %s | next:%p\n", p, node_type_to_string[p->type], p->siblings.next);
                }

                AH_PRINT("Assigning params to fun [%p]...\n", fun);
                node_identifier *i;
                p = list_entry(param_list->children, ast, siblings);
                list_for_each_entry(i, fun->args, siblings) {
                    AH_PRINT(" PARAM: " CYAN "%s" RESET " [%p]: eval...\n", ((node_identifier*)i)->i, i);
					eval((ast *)p, st, res);
                    if (res->type == VT_NOTHING) {
                        yyerror(NULL, "Invalid parameter");
                        goto inv_param_exit; // XXX
                    }
                    if (res->type == VT_FUNCTION) {
						symtab *env = check_free_variables(local_symtab, (node_function *) res->value.f);
						if (env) {
							AH_PRINT("ENV HAVE SYMBOLS, RETURNING CLOSURE\n");
							res->type = VT_CLOSURE;
							res->value.c.fun = (node_function *)res->value.f;
							res->value.c.env = env;
						} else {
							AH_PRINT("ENV IS EMPTY, RETURNING FUNCTION\n");
						}
					}
                    AH_PRINT(" PARAM: " GREEN "%s" RESET " [%p]: adding to %p\n", ((node_identifier*)i)->i, i, local_symtab);
                    set_value(sym_add(local_symtab, ((node_identifier*)i)->i), res);
                    get_value(sym_lookup(local_symtab, ((node_identifier*)i)->i), res);
                    p = list_next_entry(p,siblings);
                }
                list_for_each_entry(p, fun->children, siblings) {
                    eval((ast *)p, local_symtab, res);
                    // XXX do this just for the last p (ie the returned value)
                    AH_PRINT("p> [%p] type: %s\n", p, node_type_to_string[p->type]);
                    if (res->type == VT_FUNCTION) {
						symtab *env = check_free_variables(local_symtab, (node_function *) res->value.f);
						if (env) {
							AH_PRINT("ENV HAVE SYMBOLS, RETURNING CLOSURE\n");
							res->type = VT_CLOSURE;
							res->value.c.fun = (node_function *)res->value.f;
							res->value.c.env = env;
						} else {
							AH_PRINT("ENV IS EMPTY, RETURNING FUNCTION\n");
						}
					}
                }
                inv_param_exit:
                    free_symtab(local_symtab);
                break;
            }
            default:
                yyerror(NULL, "unknown node type");
                // TODO deberia cambiar res?
		}
	} else {
		printf(RED "FATAL ERROR: eval null\n" RESET);
        exit(EXIT_FAILURE);
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
    NIL->type = NT_NIL;

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
            	case VT_BOOLEAN:
            		printf("==> %s\n", res.value.b ? "true" : "false");
            		break;
            	case VT_NUMERIC:
            		printf("==> %f\n", res.value.n);
            		break;
            	case VT_FUNCTION:
            		printf("==> function [%p]\n", res.value.f);
            		break;
            	case VT_CLOSURE:
            		printf("==> closure [%p]\n", res.value.c.fun);
            		break;
            	case VT_NOTHING:
            		break;
            	default:
            		AH_PRINTX("unknown value type\n");
            }
        }

        //free_ast(&a);

        free(input);
    }

    puts("good-bye :-)");

	return EXIT_SUCCESS;
}

