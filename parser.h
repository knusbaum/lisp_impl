#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct object object;
typedef struct cons cons;

enum obj_type {
    O_SYM,
    O_STR,
    O_NUM,
    O_CONS,
    O_FN_NATIVE
};


/** Cons Operations **/
//cons *new_cons();
//object *car(cons *);
//void setcar(cons *c, object *o);
//object *cdr(cons *);
//void setcdr(cons *c, object *o);
//void print_cons(cons *c);

/** Object Operations **/
object *new_object(enum obj_type t, void *o);
object *new_object_long(long l);
enum obj_type otype(object *o);
string *oval_symbol(object *o);
string *oval_string(object *o);
//cons *oval_cons(object *o);
long oval_long(object *o);
object *(*oval_native(object *o))(void *, void *);
object *ocar(object *o);
object *ocdr(object *o);
void print_object(object *o);

/** Symbol Operations **/
object *intern(string *symname);
object *obj_nil();

/** Parser Operations **/
object *next_form();

#endif
