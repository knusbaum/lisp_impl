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
object *subtract(context *c, object *arglist);
object *multiply(context *c, object *arglist);
object *divide(context *c, object *arglist);
object *length(context *c, object *arglist);
object *set(context *c, object *arglist);
object *quote(context *c, object *arglist);
object *let(context *c, object *arglist);
object *eq(context *c, object *arglist);
object *fn(context *c, object *arglist);
object *lisp_if(context *c, object *arglist);
object *and(context *c, object *arglist);
object *or(context *c, object *arglist);
object *num_gt(context *c, object *arglist);
object *num_lt(context *c, object *arglist);
object *num_eq(context *c, object *arglist);

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
    bind_native_fn(c, interns("-"), subtract);
    bind_native_fn(c, interns("*"), multiply);
    bind_native_fn(c, interns("/"), divide);
    bind_native_fn(c, interns("SET"), set);
    bind_native_fn(c, interns("LENGTH"), length);
    bind_native_fn(c, interns("QUOTE"), quote);
    bind_native_fn(c, interns("LET"), let);
    bind_native_fn(c, interns("EQ"), eq);
    bind_native_fn(c, interns("FN"), fn);
    bind_native_fn(c, interns("IF"), lisp_if);
    bind_native_fn(c, interns("AND"), and);
    bind_native_fn(c, interns("OR"), or);
    bind_native_fn(c, interns(">"), num_gt);
    bind_native_fn(c, interns("<"), num_lt);
    bind_native_fn(c, interns("="), num_eq);
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

context *push_context(context *curr) {
    context *c = new_context();
    c->parent = curr;
    return c;
}

context *pop_context(context *curr) {
    context *ret = curr->parent;
    free_context(curr);
    return ret;
}

void free_context(context *c) {
    map_destroy(c->vars);
    map_destroy(c->funcs);
    free(c);
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

object *sum_rec(context *c, object *arglist, long v) {
    if(arglist == obj_nil()) {
        object *o = new_object_long(v);
        return o;
    }

    object *next = ocar(arglist);
    next = eval(c, next);
    long next_val = oval_long(next);
    v += next_val;
    return sum_rec(c, ocdr(arglist), v);
}

object *sum(context *c, object *arglist) {
    return sum_rec(c, arglist, 0);
}

object *subtract(context *c, object *arglist) {
    (void)c;
    assert_length_gt_or_eq(arglist, 1);

    long val = oval_long(eval(c, ocar(arglist)));
    for(object *curr = ocdr(arglist); curr != obj_nil(); curr = ocdr(curr)) {
        object *curr_val = eval(c, ocar(curr));
        val -= oval_long(curr_val);
    }
    return new_object_long(val);
}

object *multiply(context *c, object *arglist) {
    (void)c;
    assert_length_gt_or_eq(arglist, 1);

    long val = oval_long(eval(c, ocar(arglist)));
    for(object *curr = ocdr(arglist); curr != obj_nil(); curr = ocdr(curr)) {
        object *curr_val = eval(c, ocar(curr));
        val *= oval_long(curr_val);
    }
    return new_object_long(val);
}
object *divide(context *c, object *arglist) {
    (void)c;
    assert_length_gt_or_eq(arglist, 1);
    
    long val = oval_long(eval(c, ocar(arglist)));
    for(object *curr = ocdr(arglist); curr != obj_nil(); curr = ocdr(curr)) {
        object *curr_val = eval(c, ocar(curr));
        val /= oval_long(curr_val);
    }
    return new_object_long(val);
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

object *lisp_if(context *c, object *arglist) {
    assert_length(arglist, 3);

    object *ret = obj_nil();
    object *cond = eval(c, ocar(arglist));
    if(cond != obj_nil()) {
        ret = eval(c, ocar(ocdr(arglist)));
    }
    else {
        ret = eval(c, ocar(ocdr(ocdr(arglist))));
    }
    return ret;
}

object *and(context *c, object *arglist) {
    object *ret = obj_nil();
    for(object *curr = arglist; curr != obj_nil(); curr = ocdr(curr)) {
        ret = eval(c, ocar(curr));
        if(!ret) return obj_nil();
    }
    return ret;
}

object *or(context *c, object *arglist) {
    object *ret = obj_nil();
    for(object *curr = arglist; curr != obj_nil(); curr = ocdr(curr)) {
        ret = eval(c, ocar(curr));
        if(ret != obj_nil()) return ret;
    }
    return obj_nil();
}

object *num_gt(context *c, object *arglist) {
    (void)c;
    assert_length(arglist, 2);
    object *x = eval(c, ocar(arglist));
    object *y = eval(c, ocar(ocdr(arglist)));

    if(oval_long(x) > oval_long(y)) {
        return obj_t();
    }
    else {
        return obj_nil();
    }
}

object *num_lt(context *c, object *arglist) {
    (void)c;
    assert_length(arglist, 2);
    object *x = eval(c, ocar(arglist));
    object *y = eval(c, ocar(ocdr(arglist)));

    if(oval_long(x) < oval_long(y)) {
        return obj_t();
    }
    else {
        return obj_nil();
    }
}

object *num_eq(context *c, object *arglist) {
    (void)c;
    assert_length(arglist, 2);
    object *x = eval(c, ocar(arglist));
    object *y = eval(c, ocar(ocdr(arglist)));
 
    if(oval_long(x) == oval_long(y)) {
        return obj_t();
    }
    else {
        return obj_nil();
    }
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

static object *apply(context *c, object *fsym, object *arglist) {

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

static object *eval_sym(context *c, object *o) {
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
