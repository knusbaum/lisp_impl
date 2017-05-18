#include <stdlib.h>
#include <stdio.h>
#include "lisp.h"
#include "map.h"

struct context {
    map_t *vars;
    map_t *funcs;
    context *parent;
};

/** Built-ins **/
object *sum(context *c, object *arglist);
object *length(context *c, object *arglist);
object *set(context *c, object *arglist);
object *quote(context *c, object *arglist);
object *let(context *c, object *arglist);

object *lookup_var(context *c, object *sym) {
    object *o = map_get(c->vars, sym);
    if(!o && c->parent) {
        return lookup_var(c->parent, sym);
    }
    return o;
}

object *bind_var(context *c, object *sym, object *var) {
    map_put(c->vars, sym, var);
    return var;
}

object *lookup_fn(context *c, object *sym) {
    object *o = map_get(c->funcs, sym);
    if(!o && c->parent) {
        return lookup_fn(c->parent, sym);
    }
    return o;
}

int sym_equal(void *a, void *b) {
    return a == b;
}

void init_context_funcs(context *c) {
    object *sym;
    sym = intern(new_string_copy("+"));
    map_put(c->funcs, sym, new_object(O_FN_NATIVE, sum));

    sym = intern(new_string_copy("SET"));
    map_put(c->funcs, sym, new_object(O_FN_NATIVE, set));

    sym = intern(new_string_copy("LENGTH"));
    map_put(c->funcs, sym, new_object(O_FN_NATIVE, length));

    sym = intern(new_string_copy("QUOTE"));
    map_put(c->funcs, sym, new_object(O_FN_NATIVE, quote));

    sym = intern(new_string_copy("LET"));
    map_put(c->funcs, sym, new_object(O_FN_NATIVE, let));
}

context *new_context() {
    context *c = malloc(sizeof (context));
    c->vars = map_create(sym_equal);
    c->funcs = map_create(sym_equal);
    c->parent = NULL;
    return c;
}

void free_context(context *c) {
    map_destroy(c->vars);
    map_destroy(c->funcs);
    free(c);
}

object *sum_rec(context *c, object *arglist, long v) {
    if(arglist == obj_nil()) {
        object *o = new_object_long(v);
        return o;
    }

    object *next = ocar(arglist);
    next = eval(c, next);
    if(otype(next) != O_NUM) {
        printf("Expecting number. Got: ");
        print_object(arglist);
        printf("\n");
        abort();
    }

    long next_val = oval_long(next);
    v += next_val;
    return sum_rec(c, ocdr(arglist), v);
}

object *sum(context *c, object *arglist) {
    return sum_rec(c, arglist, 0);
}

object *length_rec(context *c, object *arglist, long v) {
    if(arglist == obj_nil()) {
        return new_object_long(v);
    }

    v++;
    object *next = ocdr(arglist);
    return length_rec(c, next, v);
}

object *length(context *c, object *arglist) {
    if(arglist == obj_nil()) {
        printf("Expected exactly 1 arg, but got none.\n");
        abort();
    }

    object *list = ocar(arglist);
    list = eval(c, list);
    printf("Evaling length on: ");
    print_object(list);
    printf("\n");
    return length_rec(c, list, 0);
}

object *set(context *c, object *arglist) {
    if(arglist == obj_nil()) {
        printf("Expected exactly 2 args, but got none.\n");
        abort();
    }

    object *var = ocar(arglist);
    object *rest = ocdr(arglist);
    object *val = ocar(rest);
    if(ocdr(rest) != obj_nil()) {
        printf("Expected exactly 2 args, but got more.\n");
        abort();
    }

    val = eval(c, val);
    bind_var(c, var, val);
    return val;
}

object *quote(context *c, object *arglist) {
    (void)c;
    return ocar(arglist);
}

void let_bindings(context *c, object *arglist) {
    for(object *frm = arglist; frm != obj_nil(); frm = ocdr(frm)) {
        object *assign = ocar(frm);
        object *sym = ocar(assign);
        object *val = eval(c, ocar(ocdr(assign)));
        bind_var(c, sym, val);
    }
}

object *let(context *c, object *arglist) {
    context *new_c = new_context();
    new_c->parent = c;
    let_bindings(new_c, ocar(arglist));


    object *res = obj_nil();
    for(object *body = ocdr(arglist); body != obj_nil(); body = ocdr(body)) {
        res = eval(new_c, ocar(body));
    }
    return res;
}

object *apply(context *c, object *fsym, object *arglist) {

    if(otype(fsym) != O_SYM) {
        printf("Expecting function. Got: ");
        print_object(fsym);
        printf("\n");
        abort();
    }

    if(otype(arglist) != O_CONS && arglist != obj_nil()) {
        printf("Expecting Arglist. Got: ");
        print_object(arglist);
        printf("\n");
        abort();
    }

    object *fn = lookup_fn(c, fsym);
    if(fn != NULL) {
        return oval_native(fn)(c, arglist);
    }

    printf("No such function: ");
    print_object(fsym);
    printf("\n");
    return obj_nil();
}

object *eval_sym(context *c, object *o) {
    object *val = lookup_var(c, o);
    if(!val) {
        printf("Error: %s is not bound.\n", string_ptr(oval_symbol(o)));
        abort();
    }
    return val;
}

object *eval(context *c, object *o) {
    object *func;

    switch(otype(o)) {
    case O_CONS:
        func = ocar(o);
        return apply(c, func, ocdr(o));
        break;
    case O_SYM:
        return eval_sym(c, o);
        break;
    default:
        return o;
        break;
    }
}
