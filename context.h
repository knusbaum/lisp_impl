#ifndef CONTEXT_H
#define CONTEXT_H

#include "lexer.h"
#include "parser.h"
#include "object.h"

typedef struct context context;

context *new_context();
context *push_context(context *curr);
context *pop_context(context *curr);
void free_context(context *c);
object *lookup_var(context *c, object *sym);
object *bind_var(context *c, object *sym, object *var);
object *lookup_fn(context *c, object *sym);
void bind_native_fn(context *c, object *sym, void (*fn)(context *, long));
void bind_fn(context *c, object *sym, object *fn);

#endif
