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
object *eq(context *c, object *arglist);
object *fn(context *c, object *arglist);

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

void bind_native_fn(context *c, object *sym, object *(*fn)(context *, object *)) {
    map_put(c->funcs, sym, new_object(O_FN_NATIVE, fn));
}

void bind_fn(context *c, object *sym, object *fn) {
    map_put(c->funcs, sym, fn);
}

void init_context_funcs(context *c) {
    bind_native_fn(c, interns("+"), sum);
    bind_native_fn(c, interns("SET"), set);
    bind_native_fn(c, interns("LENGTH"), length);
    bind_native_fn(c, interns("QUOTE"), quote);
    bind_native_fn(c, interns("LET"), let);
    bind_native_fn(c, interns("EQ"), eq);
    bind_native_fn(c, interns("FN"), fn);
}

int sym_equal(void *a, void *b) {
    return a == b;
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

long length_rec(object *arglist, long v) {
    if(arglist == obj_nil()) {
        return v;
    }

    v++;
    object *next = ocdr(arglist);
    return length_rec(next, v);
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
    return new_object_long(length_rec(list, 0));
}

void assert_length(object *list, long len) {
    long l = length_rec(list, 0);
    if(l != len) {
        printf("Expected exactly %ld argument(s), but got %ld.\n", len, l);
        abort();
    }
}

void assert_length_gt_or_eq(object *list, long len) {
    long l = length_rec(list, 0);
    if(l < len) {
        printf("Expected more than %ld argument(s), but got %ld.\n", len, l);
        abort();
    }
}

object *set(context *c, object *arglist) {
    assert_length(arglist, 2);

    object *var = ocar(arglist);
    object *rest = ocdr(arglist);
    object *val = ocar(rest);
    if(otype(var) != O_SYM) {
        printf("Cannot bind to object: ");
        print_object(var);
        printf("\n");
        abort();
    }

    val = eval(c, val);
    bind_var(c, var, val);
    return val;
}

object *quote(context *c, object *arglist) {
    (void)c;
    assert_length(arglist, 1);
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
    free_context(new_c);
    return res;
}

object *eq(context *c, object *arglist) {
    (void)c;
    assert_length(arglist, 2);
    object *o1 = ocar(arglist);
    object *o2 = ocar(ocdr(arglist));
    if(o1 == o2) {
        return obj_t();
    }
    else {
        return obj_nil();
    }
}

object *fn(context *c, object *arglist) {
    assert_length_gt_or_eq(arglist, 2);
    object *fname = ocar(arglist);
    object *fargs = ocar(ocdr(arglist));
    object *body = ocdr(ocdr(arglist));

    if(otype(fname) != O_SYM) {
        printf("Cannot bind function to: ");
        print_object(fname);
        printf("\n");
        abort();
    }
    if(otype(fargs) != O_CONS) {
        printf("Expected args as list, but got: ");
        print_object(fargs);
        printf("\n");
        abort();
    }

    bind_fn(c, fname, new_object_fn(fargs, body));
    return fname;
}

object *call_fn(context *c, object *fn, object *arglist) {
    context *fn_c = new_context();
    fn_c->parent = c;
    object *fargs = oval_fn_args(fn);
    object *fbody = oval_fn_body(fn);

    long fargs_len = length_rec(fargs, 0);
    long arglist_len = length_rec(arglist, 0);
    if(fargs_len != arglist_len) {
        printf("Expected %ld args, but got: %ld.\n", fargs_len, arglist_len);
        printf("Fargs: ");
        print_object(fargs);
        printf("\n");
        printf("FBody: ");
        print_object(fbody);
        printf("\n");
        abort();
    }

    for(object *curr_param = fargs, *curr_arg = arglist;
        curr_param != obj_nil();
        curr_param = ocdr(curr_param), curr_arg = ocdr(curr_arg)) {
        object *param = ocar(curr_param);
        object *arg = eval(c, ocar(curr_arg));
        bind_var(fn_c, param, arg);
    }

    object *ret = obj_nil();
    for(; fbody != obj_nil(); fbody = ocdr(fbody)) {
        object *form = ocar(fbody);
        ret = eval(fn_c, form);
    }

    free_context(fn_c);
    return ret;
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
        if(otype(fn) == O_FN_NATIVE) {
            return oval_native(fn)(c, arglist);
        }
        else {
            return call_fn(c, fn, arglist);
        }
    }

    printf("No such function: ");
    print_object(fsym);
    printf("\n");
    return obj_nil();
}

object *eval_sym(context *c, object *o) {
    if(o == obj_nil()) {
        return o;
    }
    if(o == obj_t()) {
        return o;
    }

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
