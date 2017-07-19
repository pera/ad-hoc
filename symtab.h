#ifndef _AH_SYMTAB_H
#define _AH_SYMTAB_H

#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define NHASH 997

symbol *sym_add(symtab*, char*);
symbol *sym_lookup(symtab*, char*);

symtab *new_symtab(symtab*);
void free_symtab(symtab*);

void set_value(symbol*, value*);
void get_value(symbol*, value*);

#endif

