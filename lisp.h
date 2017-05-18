#ifndef LISP_H
#define LISP_H

#include "lexer.h"
#include "parser.h"

typedef struct context context;

context *new_context();
void init_context_funcs(context *c);
object *eval(context *c, object *o);

#endif
