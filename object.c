#include <stdlib.h>
#include <stdio.h>
#include "object.h"
#include "map.h"

typedef struct cons cons;
struct cons {
    object *car;
    object *cdr;
};

object *car(cons *c) {
    return c->car;
}

void setcar(cons *c, object *o) {
    c->car = o;
}

object *cdr(cons *c) {
    return c->cdr;
}

void setcdr(cons *c, object *o) {
    c->cdr = o;
}

cons *new_cons() {
    cons *c = malloc(sizeof (cons));
    c->car = NULL;
    c->cdr = NULL;
    return c;
}

struct object {
    enum obj_type type;
    union {
        string *str;
        cons *c;
        long num;
        object *(*native)(void *, void *);
    };
};

object *new_object(enum obj_type t, void *o) {
    object *ob = malloc(sizeof (object));
    ob->type = t;
    switch(t) {
    case O_KEYWORD:
    case O_SYM:
    case O_STR:
        ob->str = o;
        break;
    case O_NUM:
        printf("Cannot assign a num with this function.\n");
        abort();
        break;
    case O_FN:
    case O_CONS:
        ob->c = o;
        break;
    case O_FN_NATIVE:
        ob->native = o;
        break;
    }
    return ob;
}

object *new_object_cons(object *car, object *cdr) {
    cons *c = new_cons();
    setcar(c, car);
    setcdr(c, cdr);
    return new_object(O_CONS, c);
}

object *new_object_long(long l) {
    object *ob = malloc(sizeof (object));
    ob->type = O_NUM;
    ob->num = l;
    return ob;
}

object *new_object_fn(object *args, object *body) {
    cons *cell = new_cons();
    setcar(cell, args);
    setcdr(cell, body);
    return new_object(O_FN, cell);
}

object *new_object_list(size_t len, ...) {
    va_list ap;
    va_start(ap, len);

    object *start = NULL;
    object *current = NULL;
    for(size_t i = 0; i < len; i++) {
        object *next = new_object_cons(va_arg(ap, object *), obj_nil());
        if(current) {
            osetcdr(current, next);
        }
        current = next;
        if(!start) start=current;
    }
    va_end(ap);
    return start;
}

enum obj_type otype(object *o) {
    return o->type;
}

string *oval_symbol(object *o) {
    if(o->type != O_SYM) {
        printf("Expected sym, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->str;
}

string *oval_string(object *o) {
    if(o->type != O_STR) {
        printf("Expected string, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->str;
}

long oval_long(object *o) {
    if(o->type != O_NUM) {
        printf("Expected number, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->num;
}

object *(*oval_native(object *o))(void *, void *) {
    if(o->type != O_FN_NATIVE) {
        printf("Expected number, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->native;
}

object *oval_fn_args(object *o) {
    cons *cell = o->c;
    return car(cell);
}

object *oval_fn_body(object *o) {
    cons *cell = o->c;
    return cdr(cell);
}

object *ocar(object *o) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
    }
    return car(o->c);
}

object *ocdr(object *o) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
    }
    return cdr(o->c);
}

object *osetcar(object *o, object *car) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
    }
    setcar(o->c, car);
    return car;
}

object *osetcdr(object *o, object *cdr) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
    }
    setcdr(o->c, cdr);
    return cdr;
}

static void print_cons(cons *c);

static void print_cdr(object *o) {
    switch(otype(o)) {
    case O_KEYWORD:
    case O_SYM:
        printf(" . %s", string_ptr(o->str));
        break;
    case O_STR:
        printf(" . \"%s\"", string_ptr(o->str));
        break;
    case O_NUM:
        printf(" . %ld", o->num);
        break;
    case O_CONS:
        printf(" ");
        print_cons(o->c);
        break;
    case O_FN_NATIVE:
        printf(" . #<NATIVE FUNCTION>");
        break;
    case O_FN:
        printf("#<FUNCTION @ %p>", o);
        break;
    }
}

static void print_cons(cons *c) {
    object *o = car(c);
    if(o) {
        print_object(o);
        o = cdr(c);
        if(o != obj_nil()) {
            print_cdr(o);
        }
    }
}

static void print_list(cons *c) {
    printf("(");
    print_cons(c);
    printf(")");
}

void print_object(object *o) {
    if(!o) return;
    switch(otype(o)) {
    case O_KEYWORD:
    case O_SYM:
        printf("%s", string_ptr(o->str));
        break;
    case O_STR:
        printf("\"%s\"", string_ptr(o->str));
        break;
    case O_NUM:
        printf("%ld", o->num);
        break;
    case O_CONS:
        print_list(o->c);
        break;
    case O_FN_NATIVE:
        printf("#<NATIVE FUNCTION>");
        break;
    case O_FN:
        printf("#<FUNCTION @ %p>", o);
        break;
    }
}

map_t *symbols;

object *interns(char *symname) {
    return intern(new_string_copy(symname));
}

object *intern(string *symname) {
    if(!symbols) {
        symbols = map_create((int (*)(void *, void *))string_equal);
    }

    object *s = map_get(symbols, symname);
    if(!s) {
        if(string_ptr(symname)[0] == ':') {
            s = new_object(O_KEYWORD, symname);
        }
        else {
            s = new_object(O_SYM, symname);
        }
        s->str = symname;
        map_put(symbols, symname, s);
    }
    else {
        string_free(symname);
    }
    return s;
}

object *nil;
object *obj_nil() {
    if(!nil) {
        nil = interns("NIL");
    }
    return nil;
}

object *otrue;
object *obj_t() {
    if(!otrue) {
        otrue = interns("T");
    }
    return otrue;
}
