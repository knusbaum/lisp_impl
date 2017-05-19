#include <stdlib.h>
#include <stdio.h>
#include "threaded_vm.h"
#include "lisp.h"

#define INIT_STACK 1
object **stack;
size_t s_off;
size_t stack_size;

void init() {
    stack = malloc(sizeof (object *) * INIT_STACK);
    s_off = 0;
    stack_size = INIT_STACK;


}

void push(object *o) {
    if(s_off == stack_size) {
        stack_size *= 2;
        stack = realloc(stack, stack_size * sizeof (object *));
        printf("Doubling stack to %ld\n", stack_size);
    }
    stack[s_off++] = o;
}

object *pop() {
    if(s_off == 0) return obj_nil();
    return stack[--s_off];
}

void vm_plus(context *c, long variance) {
    (void)c;
    long val = 0;
    for(int i = 0; i < variance; i++) {
        //args = new_object_cons(pop(), args);
        val += oval_long(pop());
    }
    push(new_object_long(val));
}

void call(context *c, long variance) {
    (void)variance;
    object *fsym = pop();
//    object *args = obj_nil();
    object *fn = lookup_fn(c, fsym);
    if(fn == obj_nil() || fn == NULL) {
        printf("Cannot call function: ");
        print_object(fsym);
        printf("\n");
        abort();
    }
    oval_native(fn)(c, variance);
}

void bind(context *c) {
    object *sym = pop();
    object *val = pop();
    bind_var(c, sym, val);
}

void resolve(context *c) {
    object *sym = pop();
    push(lookup_var(c, sym));
}

/** VM **/

void test() {
    init();
    char *o = (char *)0x01;
    push((object *)o);
    o++;
    push((object *)o);
    o++;
    push((object *)o);
    o++;
    push((object *)o);

    printf("Popped: %p\n", pop());
    printf("Popped: %p\n", pop());
    printf("Popped: %p\n", pop());
    printf("Popped: %p\n", pop());
}

static void compile_cons(context *c, object *o) {
    object *func = ocar(o);


    if(func == interns("LET")) {
        {
            printf("push_lex_context\n");
            c = push_context(c);
        }
        object *letlist = ocar(ocdr(o));
        for(object *frm = letlist; frm != obj_nil(); frm = ocdr(frm)) {
            object *assign = ocar(frm);
            object *sym = ocar(assign);
            compile_bytecode(c, ocar(ocdr(assign)));
            printf("push symbol %s\n", string_ptr(oval_symbol(sym)));
            push(sym);
            printf("bind\n");
            bind(c);
        }
        for(object *body = ocdr(ocdr(o)); body != obj_nil(); body = ocdr(body)) {
            compile_bytecode(c, ocar(body));
        }
        printf("pop_lex_context\n");
        c = pop_context(c);
    }
    else {
        int num_args = 0;
        for(object *curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(c, ocar(curr));
        }
        printf("push function %s\n", string_ptr(oval_symbol(func)));
        push(func);
        printf("call (%d)\n", num_args);
        call(c, num_args);
    }
}

void compile_bytecode(context *c, object *o) {

    switch(otype(o)) {
    case O_CONS:
        compile_cons(c, o);
        break;
    case O_SYM:
        printf("push sym %s\n", string_ptr(oval_symbol(o)));
        push(o);
        printf("resolve_sym\n");
        resolve(c);
        break;
    case O_STR:
        printf("push string literal %s\n", string_ptr(oval_string(o)));
        push(o);
        break;
    case O_NUM:
        printf("push num literal %ld\n", oval_long(o));
        push(o);
        break;
    default:
        printf("Something else: ");
        print_object(o);
        printf("\n");
    }
}

object *vm_eval(object *o) {
    context *c = new_context();

    bind_native_fn(c, interns("+"), vm_plus);

    compile_bytecode(c, o);
    free_context(c);
    return pop();
}
