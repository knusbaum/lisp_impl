#ifndef LISP_H
#define LISP_H

#include "lexer.h"
#include "parser.h"

typedef struct context context;

context *new_context();

object *eval(object *o, context *c);

#endif
