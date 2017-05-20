#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "threaded_vm.h"
#include "context.h"
#include "map.h"
#include "compiler.h"


static int str_eq(void *s1, void *s2) {
    return strcmp((char *)s1, (char *)s2) == 0;
}

map_t *addrs; 

map_t *get_vm_addrs();
void run_vm(context *c, compiled_chunk *cc);


#define INIT_STACK 1
object **stack;
size_t s_off;
size_t stack_size;

/** Built-ins **/
//object *sum(context *c, object *arglist);
//object *subtract(context *c, object *arglist);
//object *multiply(context *c, object *arglist);
//object *divide(context *c, object *arglist);
//object *length(context *c, object *arglist);
//object *set(context *c, object *arglist);
//object *quote(context *c, object *arglist);
//object *let(context *c, object *arglist);
//object *eq(context *c, object *arglist);
//object *fn(context *c, object *arglist);
//object *lisp_if(context *c, object *arglist);
//object *and(context *c, object *arglist);
//object *or(context *c, object *arglist);
//object *num_gt(context *c, object *arglist);
//object *num_lt(context *c, object *arglist);
//object *num_eq(context *c, object *arglist);

void vm_plus(context *c, long variance);
void vm_minus(context *c, long variance);
void vm_mult(context *c, long variance);
void vm_div(context *c, long variance);
void vm_num_eq(context *c, long variance);
void vm_list(context *c, long variance);
void vm_car(context *c, long variance);
void vm_cdr(context *c, long variance);
void vm_cons(context *c, long variance);
//void vm_macroexpand(context *c, long variance);

void vm_init(context *c) {
    stack = malloc(sizeof (object *) * INIT_STACK);
    s_off = 0;
    stack_size = INIT_STACK;

    bind_var(c, interns("NIL"), interns("NIL"));
    bind_var(c, interns("T"), interns("T"));

    bind_native_fn(c, interns("+"), vm_plus);
    bind_native_fn(c, interns("-"), vm_minus);
    bind_native_fn(c, interns("*"), vm_mult);
    bind_native_fn(c, interns("/"), vm_div);
    bind_native_fn(c, interns("="), vm_num_eq);
    bind_native_fn(c, interns("LIST"), vm_list);
    bind_native_fn(c, interns("CAR"), vm_car);
    bind_native_fn(c, interns("CDR"), vm_cdr);
    bind_native_fn(c, interns("CONS"), vm_cons);
    //bind_native_fn(c, interns("MACROEXPAND"), vm_macroexpand);

    addrs = get_vm_addrs();
}

void push(object *o) {
    if(s_off == stack_size) {
        stack_size *= 2;
        stack = realloc(stack, stack_size * sizeof (object *));
    }
//    if(s_off == 4096) {
//        printf("Blew the stack.\n");
//        abort();
//    }
    stack[s_off++] = o;
}

object *pop() {
    if(s_off == 0) return obj_nil();
    return stack[--s_off];
}

void dump_stack() {
    for(size_t i = 0; i < s_off; i++) {
        print_object(stack[i]);
        printf("\n");
    }
}

void vm_plus(context *c, long variance) {
    (void)c;
    long val = 0;
    for(int i = 0; i < variance; i++) {
        object * v = pop();
        val += oval_long(v);
    }
    push(new_object_long(val));
}

void vm_minus(context *c, long variance) {
    (void)c;
    if(variance < 1) {
        printf("Expected at least 1 argument, but got none.\n");
        abort();
    }

    long val = 0;
    for(int i = 0; i < variance - 1; i++) {
        val += oval_long(pop());
    }
    val = oval_long(pop()) - val;
    push(new_object_long(val));
}

void vm_mult(context *c, long variance) {
    (void)c;
    long val = 1;
    for(int i = 0; i < variance; i++) {
        val *= oval_long(pop());
    }
    push(new_object_long(val));
}

void vm_div(context *c, long variance) {
    if(variance < 1) {
        printf("Expected at least 1 argument, but got none.\n");
        abort();
    }
    (void)c;
    long val = 1;
    for(int i = 0; i < variance - 1; i++) {
        val *= oval_long(pop());
    }
    val = oval_long(pop()) / val;
    push(new_object_long(val));
}

void vm_num_eq(context *c, long variance) {
    (void)c;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    if(oval_long(pop()) == oval_long(pop())) {
        push(obj_t());
    }
    else {
        push(obj_nil());
    }
}

void vm_list(context *c, long variance) {
    (void)c;
    object *cons = obj_nil();
    for(int i = 0; i < variance; i++) {
        cons = new_object_cons(pop(), cons);
    }
    push(cons);
}

void vm_car(context *c, long variance) {
    (void)c;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }
    object *list = pop();
    push(ocar(list));
}

void vm_cdr(context *c, long variance) {
    (void)c;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }
    object *list = pop();
    push(ocdr(list));
}

void vm_cons(context *c, long variance) {
    (void)c;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    object *cdr = pop();
    object *car = pop();
    object *cons = new_object_cons(car, cdr);
    push(cons);
}
//void fn_call(compiled_chunk *cc, context *c, object *fn);
//void vm_macroexpand(compiled_chunk *cc, context *c, long variance) {
//    (void)c;
//    if(variance != 1) {
//        printf("Expected exactly 1 argument, but got %ld.\n", variance);
//        abort();
//    }
//    object *o = pop();
//    printf("ARG IS: ");
//    print_object(o);
//    printf("\n");
//
//    object *func = ocar(o);
//    for(object *curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
//        printf("push object: ");
//        print_object(ocar(curr));
//        printf("\n");
//        bs_push(cc, ocar(curr));
//    }
//    printf("macroexpand %s\n", string_ptr(oval_symbol(func))); ///
//    object *fn = lookup_fn(c, func);
//    fn_call(cc, c, fn);
//}

void call(context *c, long variance) {
    //object *fsym = pop();

    object *fn = pop(); //lookup_fn(c, fsym);
    if(fn == obj_nil() || fn == NULL) {
        printf("Cannot call function: ");
        print_object(fn);
        printf("\n");
        abort();
    }
    if(otype(fn) == O_FN_NATIVE) {
        oval_native(fn)(c, variance);
    }
    else if (otype(fn) == O_FN_COMPILED) {
        compiled_chunk *cc = oval_fn_compiled(fn);
        run_vm(c, cc);
    }
    else if(otype(fn) == O_MACRO) {
        compiled_chunk *cc = oval_macro_compiled(fn);
        object *result = pop();
        compile_bytecode(cc, c, result);
    }
    else {
        printf("Cannot eval this thing. not implemented: ");
        print_object(fn);
        printf("\n");
        abort();
    }
}

void bind(context *c) {
    object *sym = pop();
    object *val = pop();
    bind_var(c, sym, val);
}

void resolve(context *c) {
    object *sym = pop();
    object *val = lookup_var(c, sym);
    if(val) {
        push(val);
    }
    else {
        printf("Error: Symbol %s is not bound.\n", string_ptr(oval_symbol(sym)));
        abort();
    }
}


object *vm_eval(context *c, object *o) {
    compiled_chunk *cc = compile_form(c, o);
    
    run_vm(c, cc);
    printf("Stack length: %ld\n", s_off);
    return pop();
}


#define NEXTI {                                 \
        bs++;                                   \
        goto *bs->instr;                        \
    };

void *___vm(context *c, compiled_chunk *cc, int _get_vm_addrs) {
    if(_get_vm_addrs) {
        map_t *m = map_create(str_eq);
        map_put(m, "push", &&push);
        map_put(m, "pop", &&pop);
        map_put(m, "call", &&call);
        map_put(m, "resolve_sym", &&resolve_sym);
        map_put(m, "bind", &&bind);
        map_put(m, "push_lex_context", &&push_lex_context);
        map_put(m, "pop_lex_context", &&pop_lex_context);
        map_put(m, "go", &&go);
        map_put(m, "go_if_nil", &&go_if_nil);
        map_put(m, "exit", &&exit);
        return m;
    }

//    printf("push: (%p)\n", &&push);
//    printf("pop: (%p)\n", &&pop);
//    printf("call: (%p)\n", &&call);
//    printf("resolve_sym: (%p)\n", &&resolve_sym);
//    printf("bind: (%p)\n", &&bind);
//    printf("push_lex_context: (%p)\n", &&push_lex_context);
//    printf("pop_lex_context: (%p)\n", &&pop_lex_context);
//    printf("go: (%p)\n", &&go);
//    printf("go_if_nil: (%p)\n", &&go_if_nil);
//    printf("exit: (%p)\n", &&exit);

    struct binstr *bs = cc->bs;

    goto *bs->instr;
    return NULL;

    size_t target;

push:
    printf("%ld@%p, PUSH: ", bs - cc->bs, cc);
    print_object(bs->arg);
    printf("\n");
    push(bs->arg);
    NEXTI;
pop:
    printf("%ld@%p POP: ", bs - cc->bs, cc);
    print_object(pop());
    printf("\n");
    NEXTI;
call:
    printf("%ld@%p CALL (%ld)\n", bs - cc->bs, cc, bs->variance);
    call(c, bs->variance);
    NEXTI;
resolve_sym:
    printf("%ld@%p RESOLVE_SYM\n", bs - cc->bs, cc);
    resolve(c);
    NEXTI;
bind:
    printf("%ld@%p BIND\n", bs - cc->bs, cc);
    bind(c);
    NEXTI;
push_lex_context:
    printf("%ld@%p PUSH_LEX_CONTEXT\n", bs - cc->bs, cc);
    c = push_context(c);
    NEXTI;
pop_lex_context:
    printf("%ld@%p POP_LEX_CONTEXT\n", bs - cc->bs, cc);
    c = pop_context(c);
    NEXTI;
go:
    target = (size_t)map_get(cc->labels, bs->str);
    printf("%ld@%p GO (%s)(%ld)\n", bs - cc->bs, cc, bs->str, target);
    //bs->instr = &&go_optim;
    //bs->offset = target;
    bs = cc->bs + target;
    goto *bs->instr;
go_if_nil:
    target = (size_t)map_get(cc->labels, bs->str);
    printf("%ld@%p GO_IF_NIL (%s)(%ld) ", bs - cc->bs, cc, bs->str, target);
    if(pop() == obj_nil()) {
        printf("(jumping to %p)\n", (cc->bs + target)->instr);
        //target = (size_t)map_get(cc->labels, bs->str);
//        bs->instr = &&go_if_nil_optim;
//        bs->offset = target;
        bs = cc->bs + target;
        goto *bs->instr;
    }
    printf("(not jumping)\n");
    NEXTI;
//go_optim:
//    printf("%ld@%p GO_OPTIM (%ld)\n", bs - cc->bs, cc, bs->offset);
//    bs = cc->bs + bs->offset;
//    goto *bs->instr;
//go_if_nil_optim:
//    printf("%ld@%p GO_IF_NIL_OPTIM (%ld) ", bs - cc->bs, cc, bs->offset);
//    if(pop() == obj_nil()) {
//        printf("(jumping)\n");
//        bs = cc->bs + bs->offset;
//        goto *bs->instr;
//    }
//    printf("(not jumping)\n");
//    NEXTI;

exit:
    printf("%ld@%p EXIT\n", bs - cc->bs, cc);
    return NULL;
}

map_t *get_vm_addrs() {
    return (map_t *)___vm(NULL, NULL, 1);
}

void run_vm(context *c, compiled_chunk *cc) {
    ___vm(c, cc, 0);
}
