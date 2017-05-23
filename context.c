#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "context.h"
#include "map.h"

struct context {
    map_t *vars;
    map_t *funcs;
//    context *parent;
};

#define CSTACK_INITIAL_SIZE 8
struct context_stack {
    struct context **cstack;
    size_t cstackoff;
    size_t cstacksize;
};

context *new_context();

context_stack *context_stack_init() {
    context_stack *cs = malloc(sizeof (struct context_stack));
    cs->cstack = malloc(sizeof (context *) * CSTACK_INITIAL_SIZE);
    cs->cstacksize = CSTACK_INITIAL_SIZE;
    cs->cstackoff = 0;
    cs->cstack[0] = new_context();
    return cs;
}

object *lookup_var_off(context_stack *cs, object *sym, size_t off) {
    object *o = map_get(cs->cstack[off]->vars, sym);
    if(!o && off > 0) {
        return lookup_var_off(cs, sym, off - 1);
    }
    return o;
}

object *lookup_var(context_stack *cs, object *sym) {
    //printf("Looking up object (%p) in context: (%p)\n", sym, c);
//    object *o = map_get(cstack[cstackoff]->vars, sym);
//    if(!o && c->parent) {
//        return lookup_var(c->parent, sym);
//    }
//    return o;
    return lookup_var_off(cs, sym, cs->cstackoff);
}

object *bind_var(context_stack *cs, object *sym, object *var) {
    map_put(cs->cstack[cs->cstackoff]->vars, sym, var);
    return var;
}

object *lookup_fn(context_stack *cs, object *sym) {
//    while(c->parent != NULL) {
//        c = c->parent;
//    }
    object *o = map_get(cs->cstack[0]->funcs, sym);
//    if(!o && c->parent) {
//        return lookup_fn(c->parent, sym);
//    }
    return o;
}

void bind_native_fn(context_stack *cs, object *sym, void (*fn)(context_stack *, long)) {
    // Function bindings should always be *global*
//    while(c->parent != NULL) {
//        c = c->parent;
//    }
    object *o =  new_object(O_FN_NATIVE, fn);
    map_put(cs->cstack[0]->funcs, sym, o);
    object_set_name(o, strdup(string_ptr(oval_symbol(sym))));
}

void bind_fn(context_stack *cs, object *sym, object *fn) {
    // Function bindings should always be *global*
//    while(c->parent != NULL) {
//        c = c->parent;
//    }
    map_put(cs->cstack[0]->funcs, sym, fn);
}

int sym_equal(void *a, void *b) {
    return a == b;
}

context *new_context() {
    context *c = malloc(sizeof (context));
    c->vars = map_create(sym_equal);
    c->funcs = map_create(sym_equal);
//    c->parent = NULL;
    return c;
}

context *top_context(context_stack *cs) {
    return cs->cstack[cs->cstackoff];
}

context *push_context(context_stack *cs) {
    context *c = new_context();
    //printf("Pushing new context: (%p) onto parent: (%p)\n", c, curr);
    push_existing_context(cs, c);
//    cstackoff++;
//    cstack[cstackoff] = curr;
//    c->parent = curr;
    return c;
}

context *push_existing_context(context_stack *cs, context *existing) {
    //printf("Pushing existing context: (%p) onto parent: (%p)\n", new, curr);
//    new->parent = curr;
    cs->cstackoff++;
    if(cs->cstackoff == cs->cstacksize) {
        cs->cstacksize *= 2;
        cs->cstack = realloc(cs->cstack, cs->cstacksize * sizeof (context *));
    }
    cs->cstack[cs->cstackoff] = existing;
    return existing;
}

context *pop_context(context_stack *cs) {
    //printf("Popping context: (%p) and returning parent: (%p)\n", curr, curr->parent);
//    context *ret = curr->parent;
//    curr->parent = NULL;
    //free_context(curr);
    cs->cstackoff--;
    return cs->cstack[cs->cstackoff];
}

void free_context(context *c) {
    map_destroy(c->vars);
    map_destroy(c->funcs);
    free(c);
}

struct context_var_iterator {
    context_stack *cs;
    size_t cstackoff;
    map_iterator *var_mi;
};

context_var_iterator *iterate_vars(context_stack *cs) {
    context_var_iterator *cvi = malloc(sizeof (struct context_var_iterator));
    cvi->cs = cs;
    cvi->cstackoff = cs->cstackoff;
    cvi->var_mi = iterate_map(cs->cstack[cvi->cstackoff]->vars);
    while(cvi->var_mi == NULL) {
        if(cvi->cstackoff == 0) {
            free(cvi);
            return NULL;
        }
        cvi->cstackoff--;
        cvi->var_mi = iterate_map(cs->cstack[cvi->cstackoff]->vars);
    }
    return cvi;
}

context_var_iterator *context_var_iterator_next(context_var_iterator *cvi) {
    map_iterator *next = map_iterator_next(cvi->var_mi);
    if(!next) {
        destroy_map_iterator(cvi->var_mi);
        cvi->var_mi = NULL;
        if(cvi->cstackoff > 0) {
            cvi->cstackoff--;
            cvi->var_mi = iterate_map(cvi->cs->cstack[cvi->cstackoff]->vars);
            while(cvi->var_mi == NULL) {
                if(cvi->cstackoff == 0) {
                    //free(cvi);
                    return NULL;
                }
                cvi->cstackoff--;
                cvi->var_mi = iterate_map(cvi->cs->cstack[cvi->cstackoff]->vars);
            }
            return cvi;
        }
        else {
//            cvi->cs = NULL;
//            cvi->var_mi = NULL;
            return NULL;
        }
    }
    cvi->var_mi = next;
    return cvi;
}

struct sym_val_pair context_var_iterator_values(context_var_iterator *cvi) {
    struct sym_val_pair svp;
    struct map_pair mp = map_iterator_values(cvi->var_mi);
       
    svp.sym = mp.key;
    svp.val = mp.val;
//    printf("CONTEXT Found binding from: ");
//    print_object(svp.sym);
//    printf("(%p) to: ", svp.sym);
//    print_object(svp.val);
//    printf("(%p)\n", svp.val);
    return svp;
}

void destroy_context_var_iterator(context_var_iterator *cvi) {
    if(cvi->var_mi) {
        printf("Freeing map.\n");
        destroy_map_iterator(cvi->var_mi);
    }
    printf("Freeing CVI.\n");
    free(cvi);
}
