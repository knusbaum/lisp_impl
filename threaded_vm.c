#include <stdlib.h>
#include <stdio.h>
#include "threaded_vm.h"
#include "lisp.h"

#define INIT_STACK 1
object **stack;
size_t s_off;
size_t stack_size;

object *vm_s_quote;
object *vm_s_let;
object *vm_s_fn;
object *vm_s_if;

void vm_init(context *c) {
    stack = malloc(sizeof (object *) * INIT_STACK);
    s_off = 0;
    stack_size = INIT_STACK;

    bind_var(c, interns("NIL"), interns("NIL"));
    bind_var(c, interns("T"), interns("T"));

    vm_s_quote = interns("QUOTE");
    vm_s_let = interns("LET");
    vm_s_fn = interns("FN");
    vm_s_if = interns("IF");
}

void push(object *o) {
    if(s_off == stack_size) {
        stack_size *= 2;
        stack = realloc(stack, stack_size * sizeof (object *));
    }
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
        val += oval_long(pop());
    }
    push(new_object_long(val));
}

void vm_minus(context *c, long variance) {
    (void)c;
    //dump_stack();
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
    long val = oval_long(pop());
    for(int i = 1; i < variance; i++) {
        val /= oval_long(pop());
    }
    push(new_object_long(val));
}

void vm_num_eq(context *c, long variance) {
    (void)c;
    if(variance != 2) {
        printf("Expected exactly 2 arguments, but got %ld.\n", variance);
        abort();
    }
    if(oval_long(pop()) == oval_long(pop())) {
        //printf("push T\n");
        push(obj_t());
    }
    else {
        //printf("push NIL\n");
        push(obj_nil());
    }
}

void bind(context *c);

void fn_call(context *c, object *fn) {
    {
        //printf("push_lex_context\n");
        c = push_context(c);
    }
    object *fargs = oval_fn_args(fn);
    object *fbody = oval_fn_body(fn);

    for(object *curr_param = fargs;
        curr_param != obj_nil();
        curr_param = ocdr(curr_param)) {
        object *param = ocar(curr_param);
        object *arg = pop();
        //printf("push value: ");
        //print_object(arg);
        //printf("\n");
        push(arg);
        //printf("push symbol %s\n", string_ptr(oval_symbol(param)));
        push(param);
        //printf("bind\n");
        bind(c);
    }
    object *ret = obj_nil();
    //printf("push symbol %s\n", string_ptr(oval_symbol(ret)));
    push(ret);
    for(; fbody != obj_nil(); fbody = ocdr(fbody)) {
        //printf("pop: ");
        object *opop = pop();
        (void)opop;
        //print_object(opop);
        //printf("\n");
        object *form = ocar(fbody);
        compile_bytecode(c, form);
    }
    //printf("push value: ");
    //print_object(ret);
    //printf("\n");
    {
        //printf("pop_lex_context\n");
        c = pop_context(c);
    }
}

void call(context *c, long variance) {
    object *fsym = pop();

    object *fn = lookup_fn(c, fsym);
    if(fn == obj_nil() || fn == NULL) {
        //printf("Cannot call function: ");
        //print_object(fsym);
        //printf("\n");
        abort();
    }
    if(otype(fn) == O_FN_NATIVE) {
        oval_native(fn)(c, variance);
    }
    else {
        fn_call(c, fn);
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

/** VM **/

static void vm_let(context *c, object *o) {
    {
        //printf("push_lex_context\n");
        c = push_context(c);
    }
    object *letlist = ocar(ocdr(o));
    for(object *frm = letlist; frm != obj_nil(); frm = ocdr(frm)) {
        object *assign = ocar(frm);
        object *sym = ocar(assign);
        compile_bytecode(c, ocar(ocdr(assign)));
        //printf("push symbol %s\n", string_ptr(oval_symbol(sym)));
        push(sym);
        //printf("bind\n");
        bind(c);
    }
    for(object *body = ocdr(ocdr(o)); body != obj_nil(); body = ocdr(body)) {
        compile_bytecode(c, ocar(body));
    }
    {
        //printf("pop_lex_context\n");
        c = pop_context(c);
    }
}

static void vm_fn(context *c, object *o) {
    object *fname = ocar(ocdr(o));
    object *fargs = ocar(ocdr(ocdr(o)));
    object *body = ocdr(ocdr(ocdr(o)));
    if(otype(fname) != O_SYM) {
        //printf("Cannot bind function to: ");
        //print_object(fname);
        //printf("\n");
        abort();
    }
    if(otype(fargs) != O_CONS) {
        //printf("Expected args as list, but got: ");
        //print_object(fargs);
        //printf("\n");
        abort();
    }

    bind_fn(c, fname, new_object_fn(fargs, body));
}

void vm_if(context *c, object *o) {
    object *cond = ocdr(o);
    object *true_branch = ocdr(cond);
    object *false_branch = ocar(ocdr(true_branch));

    compile_bytecode(c, ocar(cond));
    object *cval = pop();
    //print_object(cval);
    //printf("\n");
    if(cval != obj_nil()) {
        compile_bytecode(c, ocar(true_branch));
    }
    else {
        compile_bytecode(c, false_branch);
    }
}

static void compile_cons(context *c, object *o) {
    object *func = ocar(o);

    if(func == vm_s_quote) {
        //printf("Quoted.\n");
        //printf("push: ");
        //print_object(ocar(ocdr(o)));
        //printf("\n");
        push(ocar(ocdr(o)));
    }
    else if(func == vm_s_let) {
        vm_let(c, o);
    }
    else if (func == vm_s_fn) {
        vm_fn(c, o);
    }
    else if(func == vm_s_if) {
        vm_if(c, o);
    }
    else {
        int num_args = 0;
        for(object *curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(c, ocar(curr));
        }
        //printf("push function %s\n", string_ptr(oval_symbol(func)));
        push(func);
        //printf("call (%d)\n", num_args);
        call(c, num_args);
    }
}

void compile_bytecode(context *c, object *o) {
    switch(otype(o)) {
    case O_CONS:
        compile_cons(c, o);
        break;
    case O_SYM:
        //printf("push sym %s\n", string_ptr(oval_symbol(o)));
        push(o);
        //printf("resolve_sym\n");
        resolve(c);
        break;
    case O_STR:
        //printf("push string literal %s\n", string_ptr(oval_string(o)));
        push(o);
        break;
    case O_NUM:
        //printf("push num literal %ld\n", oval_long(o));
        push(o);
        break;
    default:
        printf("Something else: ");
        print_object(o);
        printf("\n");
    }
}

object *vm_eval(context *c, object *o) {
    bind_native_fn(c, interns("+"), vm_plus);
    bind_native_fn(c, interns("-"), vm_minus);
    bind_native_fn(c, interns("*"), vm_mult);
    bind_native_fn(c, interns("/"), vm_div);
    bind_native_fn(c, interns("="), vm_num_eq);

    compile_bytecode(c, o);
    printf("Stack length: %ld\n", s_off);
    return pop();
}
