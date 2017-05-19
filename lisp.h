#ifndef LISP_H
#define LISP_H

#include "lexer.h"
#include "parser.h"

typedef struct context context;

context *new_context();
context *push_context(context *curr);
context *pop_context(context *curr);
void free_context(context *c);
object *lookup_var(context *c, object *sym);
object *bind_var(context *c, object *sym, object *var);
object *lookup_fn(context *c, object *sym);
void bind_native_fn(context *c, object *sym, object *(*fn)(context *, object *));
void bind_fn(context *c, object *sym, object *fn);

void init_context_funcs(context *c);
object *eval(context *c, object *o);

#endif
