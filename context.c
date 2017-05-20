#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "context.h"
#include "map.h"

struct context {
    map_t *vars;
    map_t *funcs;
    context *parent;
};

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

void bind_native_fn(context *c, object *sym, void (*fn)(context *, long)) {
    object *o =  new_object(O_FN_NATIVE, fn);
    map_put(c->funcs, sym, o);
    object_set_name(o, strdup(string_ptr(oval_symbol(sym))));
}

void bind_fn(context *c, object *sym, object *fn) {
    map_put(c->funcs, sym, fn);
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
