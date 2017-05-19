#ifndef OBJECT_H
#define OBJECT_H

#include <stdarg.h>
#include "lstring.h"

typedef struct object object;

enum obj_type {
    O_SYM,
    O_STR,
    O_NUM,
    O_CONS,
    O_FN,
    O_FN_NATIVE,
    O_KEYWORD
};

/** Object Operations **/
object *new_object(enum obj_type t, void *o);
object *new_object_cons(object *car, object *cdr);
object *new_object_long(long l);
object *new_object_fn(object *args, object *body);
object *new_object_list(size_t len, ...);
enum obj_type otype(object *o);
string *oval_symbol(object *o);
string *oval_string(object *o);
long oval_long(object *o);
void (*oval_native(object *o))(void *, long);
object *oval_fn_args(object *o);
object *oval_fn_body(object *o);
object *ocar(object *o);
object *ocdr(object *o);
object *osetcar(object *o, object *car);
object *osetcdr(object *o, object *cdr);
void print_object(object *o);

/** Symbol Operations **/
object *interns(char *symname);
object *intern(string *symname);

/** Important Objects **/
object *obj_nil();
object *obj_t();

#endif
