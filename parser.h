#ifndef PARSER_H
#define PARSER_H

typedef struct object object;
typedef struct cons cons;

enum obj_type {
    O_SYM,
    O_STR,
    O_NUM,
    O_CONS
};

cons *new_cons();
object *car(cons *);
void setcar(cons *c, object *o);
object *cdr(cons *);
void setcdr(cons *c, object *o);
void print_cons(cons *c);

object *new_object(enum obj_type t, void *o);
enum obj_type otype(object *o);
void print_object(object *o);

object *next_form();

#endif
