#include <stdlib.h>
#include <stdio.h>
#include "lisp.h"


struct context {
    
};

context *new_context() {
    return malloc(sizeof (context));
}

object *sum(object *arglist, long v) {
    if(arglist == obj_nil()) {
        object *o = new_object_long(v);
//        printf("Sum returning: ");
//        print_object(arglist);
//        printf("\n");
        return o;
    }

    cons *c = oval_cons(arglist);
    object *next = car(c);
//    printf("Argument: ");
//    print_object(next);
//    printf("\n");
    if(otype(next) != O_NUM) {
        printf("Expecting number. Got: ");
        print_object(arglist);
        printf("\n");
        abort();
    }

    long next_val = oval_long(next);
    v += next_val;
    return sum(cdr(c), v);
}

object *apply(object *fsym, object *arglist) {

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

    if(fsym == intern(new_string_copy("+"))) {
//        printf("Executing + on: ");
//        print_object(arglist);
//        printf("\n");
        return sum(arglist, 0);
    }

    printf("No such function: ");
    print_object(fsym);
    printf("\n");
    return obj_nil();
}

object *eval(object *o, context *c) {
    object *func;

    (void)c;
    
    switch(otype(o)) {
    case O_CONS:
        func = car(oval_cons(o));
        return apply(func, cdr(oval_cons(o)));
        break;
    default:
        return obj_nil();
        break;
    }
}
