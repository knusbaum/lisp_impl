#ifndef OBJECT_H
#define OBJECT_H

#include <stdarg.h>
#include "lstring.h"
#include "map.h"

typedef struct object object;

//** NEED REFACTORING **/
typedef struct compiled_chunk compiled_chunk;
//** END REFACTORING **/

enum obj_type {
    O_SYM,
    O_STR,
    O_NUM,
    O_CONS,
    O_FN,
    O_FN_NATIVE,
    O_KEYWORD,
    O_MACRO,
    O_FN_COMPILED,
    O_MACRO_COMPILED,
    O_STACKOFFSET
};

void object_set_name(object *o, char *name);

/** Object Operations **/
object *new_object(enum obj_type t, void *o);
object *new_object_cons(object *car, object *cdr);
object *new_object_long(long l);
object *new_object_stackoffset(long l);
object *new_object_fn(object *args, object *body);
object *new_object_fn_compiled(compiled_chunk *cc);
object *new_object_macro(object *args, object *body);
object *new_object_macro_compiled(compiled_chunk *cc);
object *new_object_list(size_t len, ...);
enum obj_type otype(object *o);
string *oval_symbol(object *o);
string *oval_keyword(object *o);
string *oval_string(object *o);
long oval_long(object *o);
long oval_stackoffset(object *o);
void (*oval_native(object *o))(void *, long);
void (*oval_native_unsafe(object *o))(void *, long);
object *oval_fn_args(object *o); // Also for getting macro args
object *oval_fn_body(object *o); // Also for getting macro body
compiled_chunk *oval_fn_compiled(object *o);
compiled_chunk *oval_fn_compiled_unsafe(object *o);
void oset_fn_compiled(object *o, compiled_chunk *cc);
compiled_chunk *oval_macro_compiled(object *o);
object *ocar(object *o);
object *ocdr(object *o);
object *osetcar(object *o, object *car);
object *osetcdr(object *o, object *cdr);
void print_object(object *o);

/** Symbol Operations **/
object *interns(char *symname);
object *intern(string *symname);
map_t *get_interned();

/** Important Objects **/
object *obj_nil();
object *obj_t();

char gc_flag(object *o);
void set_gc_flag(object *o, char f);

void destroy_object(object *o);

#endif
