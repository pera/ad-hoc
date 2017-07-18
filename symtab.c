#include "symtab.h"
#include "extra.h"

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
			AH_PRINT_ERROR("symbol \"%s\" already defined\n", sym);
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
	
	AH_PRINT_ERROR("symbol table overflow\n");
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
	AH_PRINT(YELLOW "symbol \"%s\" not found.\n" RESET, sym);

	return NULL;
}

symtab *new_symtab(symtab *parent) {
	symtab *st = malloc(sizeof(symtab));
	if (!st) {
		AH_PRINT_ERROR("unable to allocate more memory, exiting...\n");
		exit(EXIT_FAILURE);
	}

	st->head = calloc(NHASH, sizeof(symbol));
	if (!st->head) {
		AH_PRINT_ERROR("unable to allocate more memory, exiting...\n");
		exit(EXIT_FAILURE);
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

void set_value(symbol *s, value *res) {
	if (s) {
		switch (res->type) {
			case VT_BOOLEAN:
			case VT_NUMERIC:
			case VT_FUNCTION:
			case VT_THUNK:
			case VT_LIST:
				s->v_ptr = malloc(sizeof(value));
				memcpy(s->v_ptr, res, sizeof(value));
				break;
			default:
				AH_PRINTX("Can't set value: unknown type\n");
				exit(EXIT_FAILURE);
		}
	} else {
		res->type = VT_NOTHING;
	}
}

void get_value(symbol *s, value *res) {
	if (s) {
		//res->type = s->v_ptr->type;
		//res->value = s->v_ptr->value;
		*res = *s->v_ptr;
	} else {
		res->type = VT_NOTHING;
	}
}

