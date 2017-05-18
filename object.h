#ifndef OBJECT_H
#define OBJECT_H

#include "lstring.h"

typedef struct object object;
typedef struct cons cons;

enum obj_type {
    O_SYM,
    O_STR,
    O_NUM,
    O_CONS,
    O_FN_NATIVE
};

/** Object Operations **/
object *new_object(enum obj_type t, void *o);
object *new_object_cons(object *car, object *cdr);
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

#endif
