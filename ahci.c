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

void build_environment(symtab*, symtab*, symtab*, ast*);
symtab *get_environment(symtab*, const node_function *const);

void eval_error(char*, value*);
void eval_n_n_b(ast*, ast*, symtab*, value*, node_type);
void eval_n_n_n(ast*, ast*, symtab*, value*, node_type);
void eval_b_b_b(ast*, ast*, symtab*, value*, node_type);
void eval_force(ast*, symtab*, value*);
void eval_apply(ast*, ast*, symtab*, value*);
void eval(ast*, symtab*, value*);

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
					printf(YELLOW "WARNING: " RESET "Free variable \"%s\" was not previously declared.\n", ((node_identifier *)a)->i);
				} else {
					AH_PRINT("Variable's scope found, adding to environment [%p].\n", env);
					if (res.type == VT_FUNCTION && res.value.f.type == FT_NEW) {
						build_environment(parent_symtab, local_symtab, env, (ast *)res.value.f.node);
					} 
					set_value(sym_add(env, ((node_identifier *)a)->i), &res);
				}
			} else {
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
		} // no breaks!
		case NT_THUNK: {
			symtab *inner_symtab = new_symtab(local_symtab);
			list_for_each_entry(p, a->children, siblings) {
				AH_PRINT("building --> %s\n", node_type_to_string[p->type]);
				build_environment(parent_symtab, inner_symtab, env, p);
			}
			free_symtab(inner_symtab);
			break;
		}
		case NT_APPLY: {
			ast *p;
			list_for_each_entry(p, ast_left(a)->children, siblings) {
				AH_PRINT("~~~~~~~~~~~> args [%p]: %s\n", p, node_type_to_string[p->type]);
				build_environment(parent_symtab, local_symtab, env, p);
			}

			symtab *inner_symtab = new_symtab(local_symtab);
			AH_NODE_INFO(ast_right(a));
			build_environment(parent_symtab, inner_symtab, env, ast_right(a));
			free_symtab(inner_symtab);
			break;
		}
		case NT_FORCE: {
			ast *p;
			symtab *inner_symtab = new_symtab(local_symtab);
			AH_NODE_INFO(ast_right(a));
			build_environment(parent_symtab, inner_symtab, env, ast_left(a));
			free_symtab(inner_symtab);
			break;
		}
		case NT_LIST:
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
        case NT_MAP:
        case NT_FOLDL:
        case NT_FOLDR:
        case NT_FILTER:
        case NT_HEAD:
        case NT_TAIL:
        case NT_REVERSE:
        case NT_APPEND:
			// built-in
			break;
		default:
			printf(RED "FATAL ERROR: %s: unknown value type: %i (%s)\n" RESET,
			       __func__,
			       a->type,
			       node_type_to_string[a->type]);
			exit(EXIT_FAILURE);
	}
}

symtab *get_environment(symtab *parent_symtab, const node_function *const fun) {
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

	if (env) AH_PRINT(BLUE "=== (returning closure) ========\n" RESET);
	else     AH_PRINT(BLUE "=== (returning nothing) ========\n" RESET);

	return env;
}

// binary operations: numerical -> numerical -> numerical
void eval_n_n_n(ast *l, ast *r, symtab *st, value *res, node_type op) {
	double n1, n2;

	eval(l, st, res);
	if (res->type == VT_NOTHING)
		return;
	if (res->type != VT_NUMERIC) {
		eval_error(RED "TYPE ERROR: " RESET "LHS operand not numeric.\n", res);
		return;
	}
	n1 = res->value.n;

	eval(r, st, res);
	if (res->type == VT_NOTHING)
		return;
	if (res->type != VT_NUMERIC) {
		eval_error(RED "TYPE ERROR: " RESET "RHS operand not numeric.\n", res);
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
			fprintf(stderr, RED "FATAL ERROR: invalid operator\n" RESET);
			exit(EXIT_FAILURE);
	}
}

void eval_error(char *s, value *res) {
	fprintf(stderr, RED "ERROR: " RESET "%s\n", s);
	res->type = VT_NOTHING;
}

// binary operations: numerical -> numerical -> boolean
void eval_n_n_b(ast *l, ast *r, symtab *st, value *res, node_type op) {
	double n1, n2;

	eval(l, st, res);
	if (res->type == VT_NOTHING)
		return;
	if (res->type != VT_NUMERIC) {
		eval_error(RED "TYPE ERROR: " RESET "LHS operand not numeric.\n", res);
		return;
	}
	n1 = res->value.n;

	eval(r, st, res);
	if (res->type == VT_NOTHING)
		return;
	if (res->type != VT_NUMERIC) {
		eval_error(RED "TYPE ERROR: " RESET "RHS operand not numeric.\n", res);
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
			fprintf(stderr, RED "FATAL ERROR: invalid operator\n" RESET);
			exit(EXIT_FAILURE);
	}
}

// binary operations: boolean -> boolean -> boolean
void eval_b_b_b(ast *l, ast *r, symtab *st, value *res, node_type op) {
	double b1, b2;

	eval(l, st, res);
	if (res->type == VT_NOTHING)
		return;
	if (res->type != VT_BOOLEAN) {
		eval_error(RED "TYPE ERROR: " RESET "LHS operand not boolean.\n", res);
		return;
	}
	b1 = res->value.b;

	eval(r, st, res);
	if (res->type == VT_NOTHING)
		return;
	if (res->type != VT_BOOLEAN) {
		eval_error(RED "TYPE ERROR: " RESET "RHS operand not boolean.\n", res);
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
			fprintf(stderr, RED "FATAL ERROR: invalid operator\n" RESET);
			exit(EXIT_FAILURE);
	}
}

unsigned long int make_list(ast *a, symtab *st, value *res) {
	value *e;
	value_list *l;

	res->type = VT_LIST;
	res->value.l = NULL;

	if (!a->children)
		return 0;

	int n = 0;
	ast *p;
	list_for_each_entry(p, a->children, siblings) {
		AH_PRINT(" value_list [%p] type: %s\n", p, node_type_to_string[p->type]);

		e = malloc(sizeof(value));
		eval(list_entry(&p->siblings, ast, siblings), st, e);
		//if e!=nothing // TODO
		l = malloc(sizeof(value_list));
		INIT_LIST_HEAD(&l->siblings);
		l->element = e;

		if (res->value.l)
			list_add_tail(&l->siblings, &(res->value.l)->siblings);
		else
			res->value.l = l;

		n++;
	}

	return n;
}

void apply(value *fun, value_list *args, symtab *st, value *res) {
	node_function *fun_node = (node_function*)fun->value.f.node;
	symtab *local_symtab;

	if (fun->value.f.env) {
		local_symtab = new_symtab(fun->value.f.env);
		local_symtab->parent->parent = st; // XXX do not modify closure env
	} else {
		local_symtab = new_symtab(st);
	}

	value_list *a = args;
	node_identifier *i;
	list_for_each_entry(i, fun_node->args, siblings) {
		set_value(sym_add(local_symtab, ((node_identifier*)i)->i), a->element);
		a = list_next_entry(a, siblings);
	}

	ast *expr;
	list_for_each_entry(expr, fun_node->children, siblings) {
		eval(expr, local_symtab, res);
		if (res->type == VT_NOTHING)
			break;
	}

	free_symtab(local_symtab);
}

void eval_force(ast *a, symtab *st, value *res) {
	eval(a, st, res);
	if (res->type != VT_THUNK)
		return;

	ast *thunk = res->value.t.node;
	symtab *local_symtab = new_symtab(st);
	ast *p;
	list_for_each_entry(p, thunk->children, siblings) {
		eval(p, local_symtab, res);
		if (res->type == VT_FUNCTION && res->value.f.type == FT_NEW && !res->value.f.env) {
			res->value.f.env = get_environment(local_symtab, res->value.f.node);
		}
	}
	free_symtab(local_symtab);
}

void eval_apply(ast *function, ast *param_list, symtab *st, value *res) {
	eval(function, st, res);
	if (res->type == VT_NOTHING)
		return;

	if (res->type != VT_FUNCTION) {
		eval_error(RED "TYPE ERROR: " RESET "LHS operand is not a function.", res);
		return;
	}

	// Built-in functions:
	if (res->value.f.type != FT_NEW) {
		switch (res->value.f.type) {
			case FT_MAP: {
				value *res_a1 = malloc(sizeof(value));
				eval(list_entry(param_list->children, ast, siblings), st, res_a1);
				if (res_a1->type != VT_FUNCTION) {
					eval_error("Invalid type: append argument is not a function.", res);
					break;
				}

				value *res_a2 = malloc(sizeof(value));
				eval(list_entry(param_list->children->next, ast, siblings), st, res_a2);
				if (res_a2->type != VT_LIST) {
					eval_error("Invalid type: append argument is not a list.", res);
					free(res_a1);
					break;
				}

				res->value.l = NULL;

				value_list *l;
				value_list *l_new;
				list_for_each_entry(l, &(res_a2->value.l)->siblings, siblings) {
					AH_PRINT(" value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);

					l_new = malloc(sizeof(value_list));
					INIT_LIST_HEAD(&l_new->siblings);
					l_new->element = l->element; // XXX
					apply(res_a1, l_new, st, l_new->element);

					if (res->value.l)
						list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
					else
						res->value.l = l_new;
				}
				res->type = VT_LIST;
				free(res_a1);
				free(res_a2);
				break;
			}
			case FT_FOLDL:
			case FT_FOLDR: {
				value *res_a1 = malloc(sizeof(value)); // function
				eval(list_entry(param_list->children, ast, siblings), st, res_a1);
				if (res_a1->type != VT_FUNCTION) {
					eval_error("Invalid type: append argument is not a function.", res);
					break;
				}

				value *res_a2 = malloc(sizeof(value)); // accumulator
				eval(list_entry(param_list->children->next, ast, siblings), st, res_a2);
				if (res_a2->type == VT_NOTHING) {
					free(res_a1);
					break;
				}

				value *res_a3 = malloc(sizeof(value)); // list
				eval(list_entry(param_list->children->next->next, ast, siblings), st, res_a3);
				if (res_a3->type != VT_LIST) {
					eval_error("Invalid type: append argument is not a list.", res);
					free(res_a1);
					free(res_a2);
					break;
				}

				value_list params, param2;
				INIT_LIST_HEAD(&params.siblings);
				INIT_LIST_HEAD(&param2.siblings);
				list_add_tail(&params.siblings, &param2.siblings);
				value_list *l;
				if (res->value.f.type == FT_FOLDL) {
					params.element = res_a2;
					list_for_each_entry(l, &(res_a3->value.l)->siblings, siblings) {
						param2.element = l->element; // XXX
						apply(res_a1, &params, st, res);
						params.element = res;
					}
				} else {
					param2.element = res_a2;
					list_for_each_entry_reverse(l, (res_a3->value.l)->siblings.prev, siblings) {
						params.element = l->element; // XXX
						apply(res_a1, &params, st, res);
						param2.element = res;
					}
				}
				free(res_a1);
				free(res_a2);
				free(res_a3);
				break;
			}
			case FT_SCANL:
			case FT_SCANR: {
				value *res_a1 = malloc(sizeof(value)); // function
				eval(list_entry(param_list->children, ast, siblings), st, res_a1);
				if (res_a1->type != VT_FUNCTION) {
					eval_error("Invalid type: append argument is not a function.", res);
					break;
				}

				value *res_a2 = malloc(sizeof(value)); // accumulator
				eval(list_entry(param_list->children->next, ast, siblings), st, res_a2);
				if (res_a2->type == VT_NOTHING) {
					free(res_a1);
					break;
				}

				value *res_a3 = malloc(sizeof(value)); // list
				eval(list_entry(param_list->children->next->next, ast, siblings), st, res_a3);
				if (res_a3->type != VT_LIST) {
					eval_error("Invalid type: append argument is not a list.", res);
					free(res_a1);
					free(res_a2);
					break;
				}

				value_list params, param2;
				INIT_LIST_HEAD(&params.siblings);
				INIT_LIST_HEAD(&param2.siblings);
				list_add_tail(&params.siblings, &param2.siblings);
				value_list *l;
				value_list *l_new;
				if (res->value.f.type == FT_SCANL) {
					res->value.l = NULL;
					params.element = res_a2;
					list_for_each_entry(l, &(res_a3->value.l)->siblings, siblings) {
						param2.element = l->element; // XXX
						l_new = malloc(sizeof(value_list));
						l_new->element = malloc(sizeof(value));
						INIT_LIST_HEAD(&l_new->siblings);
						apply(res_a1, &params, st, l_new->element);
						params.element = l_new->element;

						if (res->value.l)
							list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
						else
							res->value.l = l_new;
					}
				} else {
					res->value.l = NULL;
					param2.element = res_a2;
					list_for_each_entry_reverse(l, (res_a3->value.l)->siblings.prev, siblings) {
						params.element = l->element; // XXX
						l_new = malloc(sizeof(value_list));
						l_new->element = malloc(sizeof(value));
						INIT_LIST_HEAD(&l_new->siblings);
						apply(res_a1, &params, st, l_new->element);
						param2.element = l_new->element;

						if (res->value.l)
							list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
						else
							res->value.l = l_new;
					}
				}
				res->type = VT_LIST;
				free(res_a1);
				free(res_a2);
				free(res_a3);
				break;
			}
			case FT_FILTER: {
				value *res_a1 = malloc(sizeof(value));
				eval(list_entry(param_list->children, ast, siblings), st, res_a1);
				if (res_a1->type != VT_FUNCTION) {
					eval_error("Invalid type: append argument is not a function.", res);
					break;
				}

				value *res_a2 = malloc(sizeof(value));
				eval(list_entry(param_list->children->next, ast, siblings), st, res_a2);
				if (res_a2->type != VT_LIST) {
					eval_error("Invalid type: append argument is not a list.", res);
					free(res_a1);
					break;
				}

				res->value.l = NULL;

				value_list param;
				INIT_LIST_HEAD(&param.siblings);
				value_list *l;
				value_list *l_new;
				value res_tmp;
				list_for_each_entry(l, &(res_a2->value.l)->siblings, siblings) {
					AH_PRINT(" value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);

					param.element = l->element;
					apply(res_a1, &param, st, &res_tmp);
					if (res_tmp.type == VT_BOOLEAN && !res_tmp.value.b) // TODO typecheck the function instead
						continue;
					l_new = malloc(sizeof(value_list));
					INIT_LIST_HEAD(&l_new->siblings);
					l_new->element = param.element; // XXX

					if (res->value.l)
						list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
					else
						res->value.l = l_new;
				}
				res->type = VT_LIST;
				free(res_a1);
				free(res_a2);
				break;
			}
			case FT_HEAD: {
				eval(list_entry(param_list->children, ast, siblings), st, res);
				if (res->type != VT_LIST) {
					eval_error("Invalid type: reverse argument is not a list.", res);
					break;
				}
				if (!res->value.l) {
					eval_error("List is empty.", res);
					break;
				}
				res->type = (res->value.l)->element->type;
				res->value = (res->value.l)->element->value;
				break;
			}
			case FT_TAIL: {
				value *res_tmp = malloc(sizeof(value));
				eval(list_entry(param_list->children, ast, siblings), st, res_tmp);
				if (res_tmp->type != VT_LIST) {
					eval_error("Invalid type: reverse argument is not a list.", res);
					break;
				}

				res->value.l = NULL;

				value_list *l;
				value_list *l_new;
				list_for_each_entry(l, &(res_tmp->value.l)->siblings, siblings) {
					if (l == res_tmp->value.l)
						continue;
					AH_PRINT(" value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);

					l_new = malloc(sizeof(value_list));
					INIT_LIST_HEAD(&l_new->siblings);
					l_new->element = l->element; // XXX

					if (res->value.l)
						list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
					else
						res->value.l = l_new;
				}
				res->type = VT_LIST;
				free(res_tmp);
				break;
			}
			case FT_REVERSE: {
				value *res_tmp = malloc(sizeof(value));
				eval(list_entry(param_list->children, ast, siblings), st, res_tmp);
				if (res_tmp->type != VT_LIST) {
					eval_error("Invalid type: reverse argument is not a list.", res);
					break;
				}

				res->value.l = NULL;

				value_list *l;
				value_list *l_new;
				list_for_each_entry(l, &(res_tmp->value.l)->siblings, siblings) {
					AH_PRINT(" value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);

					l_new = malloc(sizeof(value_list));
					INIT_LIST_HEAD(&l_new->siblings);
					l_new->element = l->element; // XXX

					if (res->value.l)
						list_add(&l_new->siblings, &(res->value.l)->siblings);
					else
						res->value.l = l_new;
				}
				res->type = VT_LIST;
				res->value.l = l_new;
				free(res_tmp);
				break;
			}
			case FT_APPEND: {
				value *res_a1 = malloc(sizeof(value));
				eval(list_entry(param_list->children, ast, siblings), st, res_a1);
				if (res_a1->type != VT_LIST) {
					eval_error("Invalid type: append argument is not a list.", res);
					break;
				}

				value *res_a2 = malloc(sizeof(value));
				eval(list_entry(param_list->children->next, ast, siblings), st, res_a2);
				if (res_a2->type != VT_LIST) {
					eval_error("Invalid type: append argument is not a list.", res);
					free(res_a1);
					break;
				}

				res->value.l = NULL;

				value_list *l;
				value_list *l_new;
				list_for_each_entry(l, &(res_a1->value.l)->siblings, siblings) {
					AH_PRINT(" value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);

					l_new = malloc(sizeof(value_list));
					INIT_LIST_HEAD(&l_new->siblings);
					l_new->element = l->element; // XXX

					if (res->value.l)
						list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
					else
						res->value.l = l_new;
				}
				list_for_each_entry(l, &(res_a2->value.l)->siblings, siblings) {
					AH_PRINT(" value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);

					l_new = malloc(sizeof(value_list));
					INIT_LIST_HEAD(&l_new->siblings);
					l_new->element = l->element; // XXX

					if (res->value.l)
						list_add_tail(&l_new->siblings, &(res->value.l)->siblings);
					else
						res->value.l = l_new;
				}
				res->type = VT_LIST;
				free(res_a1);
				free(res_a2);
				break;
			}
			case FT_LENGTH: {
				value *res_tmp = malloc(sizeof(value));
				eval(list_entry(param_list->children, ast, siblings), st, res_tmp);
				if (res_tmp->type != VT_LIST) {
					eval_error("Invalid type: reverse argument is not a list.", res);
					break;
				}

				int length = 0;
				value_list *l;
				list_for_each_entry(l, &(res_tmp->value.l)->siblings, siblings) {
					length++;
				}
				res->type = VT_NUMERIC;
				res->value.n = length;
				break;
			}
			default:
				eval_error("Unimplemented function type.", res);
		}
		return;
	}

	value args; // TODO deep free
	value_list *l;
	if (!make_list(param_list, st, &args)) {
		fprintf(stderr, "FATAL ERROR Empty param_list.");
		exit(EXIT_FAILURE);
	}

	list_for_each_entry(l, &(args.value.l)->siblings, siblings) {
		AH_PRINT("ARGS: value_list [%p] type: %s\n", l, value_type_to_string[l->element->type]);
	}

	apply(res, args.value.l, st, res);
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
				if (res->type == VT_NOTHING)
					return;
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
				if (res->type == VT_NOTHING)
					return;
				if (res->type != VT_BOOLEAN) {
					eval_error(RED "TYPE ERROR: " RESET "operand not boolean.\n", res);
				} else {
					res->value.b = !res->value.b;
				}
				break;
			case NT_IF: {
				eval(((node_if*)a)->expr, st, res);
				if (res->type == VT_NOTHING)
					return;
				if (res->type != VT_BOOLEAN) {
					eval_error("Invalid type for if expression.", res);
				} else {
					if (res->value.b) {
						eval_force(((node_if*)a)->t, st, res);
					} else {
						eval_force(((node_if*)a)->f, st, res);
					}
				}
				break;
			}
			case NT_ASSIGNMENT:
				eval(((node_assignment *)a)->v, st, res);
				if (res->type == VT_NOTHING)
					return;
				set_value(sym_add(st, ((node_assignment *)a)->i), res);
				break;
			case NT_IDENTIFIER: {
				symbol *s = sym_lookup(st, ((node_identifier *)a)->i);
				if (s) {
					get_value(s, res);
				} else {
					eval_error("symbol not found", res);
				}
				break;
			}
			case NT_THUNK:
				res->type = VT_THUNK;
				res->value.t.node = a;
				res->value.t.env = NULL; // get_environment(st, res->value.t.node); // TODO
				break;
			case NT_FORCE:
				eval_force(ast_left(a), st, res);
				break;
			case NT_FUNCTION: {
				ast *p;
				list_for_each_entry(p, ((node_function *)a)->args, siblings) {
					AH_PRINT(" args [%p]: %s\n", p, ((node_identifier*)p)->i);
				}
				list_for_each_entry(p, ((node_function *)a)->children, siblings) {
					AH_PRINT(" expr_list [%p] type: %s\n", p, node_type_to_string[p->type]);
				}
				res->type = VT_FUNCTION;
				res->value.f.type = FT_NEW;
				res->value.f.node = (node_function *)a;
				res->value.f.env = get_environment(st, res->value.f.node);
				break;
			}
			case NT_APPLY:
				eval_apply(ast_right(a), ast_left(a), st, res);
				break;
			case NT_LIST:
				make_list(a, st, res);
				break;
			case NT_MAP: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_MAP;
				break;
			}
			case NT_FOLDL: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_FOLDL;
				break;
			}
			case NT_FOLDR: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_FOLDR;
				break;
			}
			case NT_SCANL: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_SCANL;
				break;
			}
			case NT_SCANR: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_SCANR;
				break;
			}
			case NT_FILTER: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_FILTER;
				break;
			}
			case NT_HEAD: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_HEAD;
				break;
			}
			case NT_TAIL: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_TAIL;
				break;
			}
			case NT_REVERSE: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_REVERSE;
				break;
			}
			case NT_APPEND: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_APPEND;
				break;
			}
			case NT_LENGTH: {
				res->type = VT_FUNCTION;
				res->value.f.type = FT_LENGTH;
				break;
			}
			default:
				eval_error("unknown node type", res);
		}
	} else {
		printf(RED "FATAL ERROR: eval null\n" RESET);
		exit(EXIT_FAILURE);
	}
}

void print_value(value *v) {
	void print_value_(value *v) {
		switch (v->type) {
			case VT_BOOLEAN:
				printf("%s", v->value.b ? "true" : "false");
				break;
			case VT_NUMERIC:
				printf("%f", v->value.n);
				break;
			case VT_FUNCTION:
				printf("function [%p] %s", v->value.f.node, v->value.f.env ? "(closure)" : "");
				break;
			case VT_THUNK:
				printf("thunk [%p] %s", v->value.t.node, v->value.t.env ? "(closure)" : "");
				break;
			case VT_LIST: {
				printf("{ ");
				value_list *p;
				list_for_each_entry(p, &(v->value.l)->siblings, siblings) {
					print_value_(p->element);
					printf(" ");
				}
				printf("}");
				break;
			}
			case VT_NOTHING:
				break;
			default:
				AH_PRINTX("unknown value type\n");
		}
	}
	printf("==> ");
	print_value_(v);
	printf("\n");
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
			print_value(&res);
		}

		free_ast(&a);

		free(input);
	}

	puts("good-bye :-)");

	return EXIT_SUCCESS;
}

