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

#define INIT_STACK 4096
object **stack;
size_t s_off;
size_t stack_size;

static inline void vm_mult(context_stack *cs, long variance);
static inline void vm_div(context_stack *cs, long variance);
void vm_num_eq(context_stack *cs, long variance);
void vm_num_gt(context_stack *cs, long variance);
void vm_num_lt(context_stack *cs, long variance);
void vm_list(context_stack *cs, long variance);
void vm_append(context_stack *cs, long variance);
void vm_splice(context_stack *cs, long variance);
void vm_car(context_stack *cs, long variance);
void vm_cdr(context_stack *cs, long variance);
void vm_cons(context_stack *cs, long variance);
void vm_length(context_stack *cs, long variance);
void vm_eq(context_stack *cs, long variance);
void vm_compile(context_stack *cs, long variance);
void vm_compile_fn(context_stack *cs, long variance);
void vm_compile_macro(context_stack *cs, long variance);
void vm_eval(context_stack *cs, long variance);
void vm_read(context_stack *cs, long variance);
void vm_print(context_stack *cs, long variance);
//void vm_macroexpand(context_stack *cs, long variance);

void vm_init(context_stack *cs) {
    stack = malloc(sizeof (object *) * INIT_STACK);
    s_off = 0;
    stack_size = INIT_STACK;

    bind_var(cs, interns("NIL"), interns("NIL"));
    bind_var(cs, interns("T"), interns("T"));

    bind_native_fn(cs, interns(">"), vm_num_gt);
    bind_native_fn(cs, interns("<"), vm_num_lt);
    bind_native_fn(cs, interns("LIST"), vm_list);
    bind_native_fn(cs, interns("APPEND"), vm_append);
    bind_native_fn(cs, interns("SPLICE"), vm_splice);
    bind_native_fn(cs, interns("CAR"), vm_car);
    bind_native_fn(cs, interns("CDR"), vm_cdr);
    bind_native_fn(cs, interns("CONS"), vm_cons);
    bind_native_fn(cs, interns("LENGTH"), vm_length);
    bind_native_fn(cs, interns("EQ"), vm_eq);
    bind_native_fn(cs, interns("COMPILE-FN"), vm_compile_fn);
    bind_native_fn(cs, interns("COMPILE-MACRO"), vm_compile_macro);
    bind_native_fn(cs, interns("EVAL"), vm_eval);
    bind_native_fn(cs, interns("READ"), vm_read);
    bind_native_fn(cs, interns("PRINT"), vm_print);
    //bind_native_fn(cs, interns("MACROEXPAND"), vm_macroexpand);

    addrs = get_vm_addrs();
}

static inline void __push(object *o) {
    if(s_off == stack_size) {
        stack_size *= 2;
        stack = realloc(stack, stack_size * sizeof (object *));
    }
    stack[s_off++] = o;
}

static inline object *__pop() {
    if(s_off == 0) return obj_nil();
    return stack[--s_off];
}

void push(object *o) {
    __push(o);
}

object *pop() {
    return __pop();
}

void dump_stack() {
    for(size_t i = 0; i < s_off; i++) {
        print_object(stack[i]);
        printf("\n");
    }
    printf("\n---------------------\n");
}

static inline void vm_mult(context_stack *cs, long variance) {
    (void)cs;
    long val = 1;
    for(int i = 0; i < variance; i++) {
        val *= oval_long(__pop());
    }
    __push(new_object_long(val));
}

static inline void vm_div(context_stack *cs, long variance) {
    if(variance < 1) {
        printf("Expected at least 1 argument, but got none.\n");
        abort();
    }
    (void)cs;
    long val = 1;
    for(int i = 0; i < variance - 1; i++) {
        val *= oval_long(__pop());
    }
    val = oval_long(__pop()) / val;
    __push(new_object_long(val));
}

void vm_num_eq(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    if(oval_long(__pop()) == oval_long(__pop())) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_num_gt(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    // args are reversed.
    if(oval_long(__pop()) <= oval_long(__pop())) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_num_lt(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    // args are reversed.
    if(oval_long(__pop()) >= oval_long(__pop())) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_list(context_stack *cs, long variance) {
    (void)cs;
    object *cons = obj_nil();
    for(int i = 0; i < variance; i++) {
        cons = new_object_cons(__pop(), cons);
    }
    __push(cons);
}

void vm_append(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }

    (void)cs;
    object *cons = new_object_cons(__pop(), obj_nil());
    object *target = __pop();
    if(target == obj_nil()) {
        __push(cons);
        return;
    }
    object *curr = target;
    while(ocdr(curr) != obj_nil()) {
        curr=ocdr(curr);
    }
    osetcdr(curr, cons);
    __push(target);
}

void vm_splice(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }

    (void)cs;
    object *to_splice = __pop();
    object *target = __pop();
    if(target == obj_nil()) {
        __push(to_splice);
        return;
    }
    object *curr = target;
    while(ocdr(curr) != obj_nil()) {
        curr=ocdr(curr);
    }
    osetcdr(curr, to_splice);
    __push(target);
}

void vm_car(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }
    object *list = __pop();
    __push(ocar(list));
}

void vm_cdr(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }
    object *list = __pop();
    __push(ocdr(list));
}

void vm_cons(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    object *cdr = __pop();
    object *car = __pop();
    object *cons = new_object_cons(car, cdr);
    __push(cons);
}

void vm_length(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }

    long len = 0;
    for(object *curr = __pop(); curr != obj_nil(); curr = ocdr(curr)) {
        len++;
    }
    __push(new_object_long(len));
}

void vm_eq(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    if(__pop() == __pop()) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
}

void vm_compile_fn(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    object *fname = __pop();
    object *uncompiled_fn = __pop();
    compiled_chunk *fn_cc = new_compiled_chunk();
    fn_cc->c = top_context(cs);
    object *fn = new_object_fn_compiled(fn_cc);
    printf("Compiling fn %s into cc: %p\n", string_ptr(oval_symbol(fname)), fn_cc);
    bind_fn(cs, fname, fn);
    compile_fn(fn_cc, cs, uncompiled_fn);
}

void vm_compile_macro(context_stack *cs, long variance) {
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    object *fname = __pop();
    object *uncompiled_fn = __pop();
    compiled_chunk *fn_cc = new_compiled_chunk();
    fn_cc->c = top_context(cs);
    object *macro = new_object_macro_compiled(fn_cc);
    printf("Compiling fn %s into cc: %p\n", string_ptr(oval_symbol(fname)), fn_cc);
    bind_fn(cs, fname, macro);
    compile_fn(fn_cc, cs, uncompiled_fn);
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


void call(context_stack *cs, long variance) {
    object *fn = __pop();
    if(fn == obj_nil() || fn == NULL) {
        printf("Cannot call function: ");
        print_object(fn);
        printf("\n");
        abort();
    }
    if (otype(fn) == O_FN_COMPILED) {
        compiled_chunk *cc = oval_fn_compiled(fn);
        if(cc->c) {
            push_existing_context(cs, cc->c);
        }
        run_vm(cs, cc);
        object *ret = __pop();
        for(int i = 0; i < variance; i++) {
            __pop();
        }
        __push(ret);
        if(cc->c) {
            pop_context(cs);
        }
    }
    else if(otype(fn) == O_FN_NATIVE) {
        oval_native_unsafe(fn)(cs, variance);
    }
    else if(otype(fn) == O_MACRO) {
        compiled_chunk *cc = oval_macro_compiled(fn);
        object *result = __pop();
        compile_bytecode(cc, cs, result);
    }
    else {
        printf("Cannot eval this thing. not implemented: ");
        print_object(fn);
        printf("\n");
        dump_stack();
        abort();
    }
}

void resolve(context_stack *cs) {
    object *sym = __pop();
    object *val = lookup_var(cs, sym);
    if(val) {
        __push(val);
    }
    else {
        printf("Error: Symbol %s is not bound.\n", string_ptr(oval_symbol(sym)));
        abort();
    }
}

void vm_eval(context_stack *cs, long variance) {
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }
    object *o = __pop();
    compiled_chunk *cc = compile_form(cs, o);
    run_vm(cs, cc);
    free_compiled_chunk(cc);
}

void vm_read(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 0) {
        printf("Expected exactly 0 arguments, but got %ld.\n", variance);
        abort();
    }
    object *o = next_form(NULL);
    if(o) {
        __push(o);
    }
    else {
        printf("Stream was closed.\n");
        abort();
    }
}

void vm_print(context_stack *cs, long variance) {
    (void)cs;
    if(variance != 1) {
        printf("Expected exactly 1 argument, but got %ld.\n", variance);
        abort();
    }
    print_object(__pop());
}

//        dump_stack();
//        printf("--------------------------\n");

#define NEXTI {                                 \
        bs++;                                   \
        goto *bs->instr;                        \
    };

void *___vm(context_stack *cs, compiled_chunk *cc, int _get_vm_addrs) {
    if(_get_vm_addrs) {
        map_t *m = map_create(str_eq);
        map_put(m, "chew_top", &&chew_top);
        map_put(m, "push", &&push);
        map_put(m, "pop", &&pop);
        map_put(m, "call", &&call);
        map_put(m, "resolve_sym", &&resolve_sym);
        map_put(m, "bind_var", &&bind_var);
        map_put(m, "bind_fn", &&bind_fn);
        map_put(m, "push_lex_context", &&push_lex_context);
        map_put(m, "pop_lex_context", &&pop_lex_context);
        map_put(m, "go", &&go);
        map_put(m, "go_if_nil", &&go_if_nil);
        map_put(m, "push_from_stack", &&push_from_stack);
        map_put(m, "add", &&add);
        map_put(m, "subtract", &&subtract);
        map_put(m, "multiply", &&multiply);
        map_put(m, "divide", &&divide);
        map_put(m, "num_eq", &&num_eq);
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
    object *ret, *sym, *val;
    long mathvar;
    long truthiness;
    int i;
chew_top:
    //printf("%ld@%p, CHEW_TOP (%ld): ", bs - cc->bs, cc, bs->offset);
    ret = __pop();
    s_off -= bs->offset;
    __push(ret);
    NEXTI;
push:
    //printf("%ld@%p, PUSH: ", bs - cc->bs, cc);
    //print_object(bs->arg);
    //printf("\n");
    __push(bs->arg);
    NEXTI;
pop:
    //printf("%ld@%p POP: ", bs - cc->bs, cc);
    //print_object(__pop());
    //printf("\n");
    __pop();
    NEXTI;
call:
    //printf("%ld@%p CALL (%ld)\n", bs - cc->bs, cc, bs->variance);
    call(cs, bs->variance);
    NEXTI;
resolve_sym:
    //printf("%ld@%p RESOLVE_SYM\n", bs - cc->bs, cc);
    resolve(cs);
    NEXTI;
bind_var:
    //printf("%ld@%p BIND\n", bs - cc->bs, cc);
    sym = __pop();
    val = __pop();
    bind_var(cs, sym, val);
    NEXTI;
bind_fn:
    sym = __pop();
    val = __pop();
    //printf("%ld@%p BIND\n", bs - cc->bs, cc);
    bind_fn(cs, sym, val);
    NEXTI;
push_lex_context:
    //printf("%ld@%p PUSH_LEX_CONTEXT\n", bs - cc->bs, cc);
    push_context(cs);
    NEXTI;
pop_lex_context:
    //printf("%ld@%p POP_LEX_CONTEXT\n", bs - cc->bs, cc);
    pop_context(cs);
    NEXTI;
go:
    target = (size_t)map_get(cc->labels, bs->str);
    //printf("%ld@%p (%p)GO (%s)(%ld)\n", bs - cc->bs, cc, bs, bs->str, target);
    bs->instr = &&go_optim;
    bs->offset = target;
    bs = cc->bs + target;
    goto *bs->instr;
go_if_nil:
    target = (size_t)map_get(cc->labels, bs->str);
    //printf("%ld@%p (%p)GO_IF_NIL (%s)(%ld) ", bs - cc->bs, cc, bs, bs->str, target);
    bs->instr = &&go_if_nil_optim;
    bs->offset = target;
    if(__pop() == obj_nil()) {
        //printf("(jumping to %p)\n", (cc->bs + target)->instr);
        bs = cc->bs + target;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;
go_optim:
    //printf("%ld@%p GO_OPTIM (%ld)\n", bs - cc->bs, cc, bs->offset);
    bs = cc->bs + bs->offset;
    goto *bs->instr;
go_if_nil_optim:
    //printf("%ld@%p GO_IF_NIL_OPTIM (%ld) ", bs - cc->bs, cc, bs->offset);
    if(__pop() == obj_nil()) {
        //printf("(jumping)\n");
        bs = cc->bs + bs->offset;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;
push_from_stack:
    //printf("%ld@%p PUSH_FROM_STACK (%ld)\n", bs - cc->bs, cc, s_off - 1 - bs->offset);
    __push(stack[s_off - 1 - bs->offset]);
    NEXTI;
add:
    //printf("%ld@%p ADD (%ld)\n", bs - cc->bs, cc, bs->variance);
    mathvar = 0;
    for(i = 0; i < bs->variance; i++) {
        mathvar += oval_long(__pop());
    }
    __push(new_object_long(mathvar));
    NEXTI;
subtract:
    //printf("%ld@%p SUBTRACT (%ld)\n", bs - cc->bs, cc, bs->variance);
    mathvar = 0;
    for(i = 0; i < bs->variance - 1; i++) {
        mathvar += oval_long(__pop());
    }
    mathvar = oval_long(__pop()) - mathvar;
    __push(new_object_long(mathvar));
    NEXTI;
multiply:
    //printf("%ld@%p MULTIPLY\n", bs - cc->bs, cc);
    vm_mult(cs, bs->variance);
    NEXTI;
divide:
    //printf("%ld@%p DIVIDE\n", bs - cc->bs, cc);
    vm_div(cs, bs->variance);
    NEXTI;
num_eq:
    //printf("%ld@%p NUM_EQ\n", bs - cc->bs, cc);
    truthiness = 1;
    mathvar = oval_long(__pop());
    for(long i = 0; i < bs->variance - 1; i++) {
        truthiness = truthiness && (mathvar == oval_long(__pop()));
    }
    if(truthiness) {
        __push(obj_t());
    }
    else {
        __push(obj_nil());
    }
    NEXTI;

exit:
    //printf("%ld@%p EXIT\n", bs - cc->bs, cc);
    return NULL;
}

map_t *get_vm_addrs() {
    return (map_t *)___vm(NULL, NULL, 1);
}

void run_vm(context_stack *cs, compiled_chunk *cc) {
    ___vm(cs, cc, 0);
}
