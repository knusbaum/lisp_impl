#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "threaded_vm.h"
#include "lisp.h"
#include "map.h"

/** CRAP TO MOVE **/

int str_eq(void *s1, void *s2) {
    return strcmp((char *)s1, (char *)s2) == 0;
}

map_t *addrs; // This shouldn't be global.

struct binstr {
    void *instr;
    union {
        object *arg;
        long variance;
        char *str;
    };
};

struct compiled_chunk {
    struct binstr *bs;
    size_t b_off;
    size_t bsize;
    map_t *labels;
};

compiled_chunk *new_compiled_chunk() {
    compiled_chunk *chunk = malloc(sizeof (compiled_chunk));
    chunk->bs = malloc(sizeof (struct binstr) * 24);
    chunk->b_off = 0;
    chunk->bsize = 24;
    chunk->labels = map_create(str_eq);
    return chunk;
}

void add_binstr_arg(compiled_chunk *c, void *instr, object *arg) {
    if(c->b_off == c->bsize) {
        c->bsize *= 2;
        c->bs = realloc(c->bs, sizeof (struct binstr) * c->bsize);
    }

    struct binstr *target = c->bs + c->b_off;
    target->instr = instr;
    target->arg=arg;
    c->b_off++;
}

void add_binstr_variance(compiled_chunk *c, void *instr, long variance) {
    if(c->b_off == c->bsize) {
        c->bsize *= 2;
        c->bs = realloc(c->bs, sizeof (struct binstr) * c->bsize);
    }

    struct binstr *target = c->bs + c->b_off;
    target->instr = instr;
    target->variance=variance;
    c->b_off++;
}

void add_binstr_str(compiled_chunk *c, void *instr, char *str) {
    if(c->b_off == c->bsize) {
        c->bsize *= 2;
        c->bs = realloc(c->bs, sizeof (struct binstr) * c->bsize);
    }

    struct binstr *target = c->bs + c->b_off;
    target->instr = instr;
    target->str=str;
    c->b_off++;
}

void free_compiled_chunk(compiled_chunk *c) {
    free(c->bs);
    map_destroy(c->labels);
    free(c);
}

void bs_push(compiled_chunk *cc, object *o) {
    printf("bs_push: ");
    print_object(o);
    printf("\n");
    add_binstr_arg(cc, map_get(addrs, "push"), o);
}

void bs_pop(compiled_chunk *cc) {
    printf("bs_pop\n");
    add_binstr_arg(cc, map_get(addrs, "pop"), NULL);
}

void bs_push_context(compiled_chunk *cc) {
    printf("bs_push_context\n");
    add_binstr_arg(cc, map_get(addrs, "push_lex_context"), NULL);
}

void bs_pop_context(compiled_chunk *cc) {
    printf("bs_pop_context\n");
    add_binstr_arg(cc, map_get(addrs, "pop_lex_context"), NULL);
}

void bs_bind(compiled_chunk *cc) {
    printf("bs_bind\n");
    add_binstr_arg(cc, map_get(addrs, "bind"), NULL);
}

void bs_resolve(compiled_chunk *cc) {
    printf("bs_resolve_context\n");
    add_binstr_arg(cc, map_get(addrs, "resolve_sym"), NULL);
}

void bs_call(compiled_chunk *cc, long variance) {
    printf("bs_call (%ld)\n", variance);
    add_binstr_variance(cc, map_get(addrs, "call"), variance);
}

void bs_exit(compiled_chunk *cc) {
    printf("bs_exit\n");
    add_binstr_arg(cc, map_get(addrs, "exit"), NULL);
}

void bs_label(compiled_chunk *cc, char *label) {
    printf("bs_label: %s\n", label);
    map_put(cc->labels, label, (void *)cc->b_off);
}

void bs_go(compiled_chunk *cc, char *label) {
    printf("bs_go: %s\n", label);
    add_binstr_str(cc, map_get(addrs, "go"), label);
}

void bs_go_if_nil(compiled_chunk *cc, char *label) {
    printf("bs_go_if_nil: %s\n", label);
    add_binstr_str(cc, map_get(addrs, "go_if_nil"), label);
}

unsigned int i;

char *mk_label() {
    char *label = malloc(12); // 10 digits + L + \0
    sprintf(label, "L%010u", i++);
    return label;
}

map_t *get_vm_addrs();
void run_vm(context *c, compiled_chunk *cc);

/** END CRAP TO MOVE **/




#define INIT_STACK 1
object **stack;
size_t s_off;
size_t stack_size;

object *vm_s_quote;
object *vm_s_backtick;
object *vm_s_comma;
object *vm_s_let;
object *vm_s_fn;
object *vm_s_if;
object *vm_s_defmacro;
object *vm_s_for;

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

    vm_s_quote = interns("QUOTE");
    vm_s_backtick = interns("BACKTICK");
    vm_s_comma = interns("COMMA");
    vm_s_let = interns("LET");
    vm_s_fn = interns("FN");
    vm_s_if = interns("IF");
    vm_s_defmacro = interns("DEFMACRO");
    vm_s_for = interns("FOR");

    addrs = get_vm_addrs();
}

void push(object *o) {
    if(s_off == stack_size) {
        stack_size *= 2;
        stack = realloc(stack, stack_size * sizeof (object *));
    }
    if(s_off == 4096) {
        printf("Blew the stack.\n");
        abort();
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

void bind(context *c);

void fn_call(compiled_chunk *cc, context *c, object *fn) {
    bs_push_context(cc);

    object *fargs = oval_fn_args(fn);
    object *fbody = oval_fn_body(fn);

    for(object *curr_param = fargs;
        curr_param != obj_nil();
        curr_param = ocdr(curr_param)) {
        object *param = ocar(curr_param);
        bs_push(cc, param);
        bs_bind(cc);
    }
    object *ret = obj_nil();
    bs_push(cc, ret);
    for(; fbody != obj_nil(); fbody = ocdr(fbody)) {
        bs_pop(cc);
        object *form = ocar(fbody);
        compile_bytecode(cc, c, form);
    }
    bs_pop_context(cc);
}

void call(context *c, long variance) {
    object *fsym = pop();

    object *fn = lookup_fn(c, fsym);
    if(fn == obj_nil() || fn == NULL) {
        printf("Cannot call function: ");
        print_object(fsym);
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

compiled_chunk *compile_fn(context *c, object *fn) {

    compiled_chunk *fn_cc = new_compiled_chunk();

    if(fn == obj_nil() || fn == NULL) {
        printf("Cannot call function: ");
        print_object(fn);
        printf("\n");
        abort();
    }
    if(otype(fn) == O_FN_NATIVE) {
        printf("call native.\n");
        //oval_native(fn)(c, variance);
    }
    else if(otype(fn) == O_FN) {
        fn_call(fn_cc, c, fn);
    }
    else if(otype(fn) == O_MACRO) {
        fn_call(fn_cc, c, fn);
        object *result = pop();
        compile_bytecode(fn_cc, c, result);
    }
    bs_exit(fn_cc);
    return fn_cc;
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

static void vm_let(compiled_chunk *cc, context *c, object *o) {
    bs_push_context(cc);

    object *letlist = ocar(ocdr(o));
    for(object *frm = letlist; frm != obj_nil(); frm = ocdr(frm)) {
        object *assign = ocar(frm);
        object *sym = ocar(assign);
        compile_bytecode(cc, c, ocar(ocdr(assign)));
        bs_push(cc, sym);
        bs_bind(cc);
    }
    for(object *body = ocdr(ocdr(o)); body != obj_nil(); body = ocdr(body)) {
        compile_bytecode(cc, c, ocar(body));
    }
    bs_pop_context(cc);
}

static void vm_fn(compiled_chunk *cc, context *c, object *o) {
    (void)cc;
    object *fname = ocar(ocdr(o));
    object *fargs = ocar(ocdr(ocdr(o)));
    object *body = ocdr(ocdr(ocdr(o)));
    if(otype(fname) != O_SYM) {
        printf("Cannot bind function to: ");
        print_object(fname);
        printf("\n");
        abort();
    }
    if(otype(fargs) != O_CONS && fargs != obj_nil()) {
        printf("Expected args as list, but got: ");
        print_object(fargs);
        printf("\n");
        abort();
    }

    compiled_chunk *fn_cc = compile_fn(c, new_object_fn(fargs, body));
    bind_fn(c, fname, new_object_fn_compiled(fn_cc));
}

void vm_if(compiled_chunk *cc, context *c, object *o) {
    object *cond = ocdr(o);
    object *true_branch = ocdr(cond);
    object *false_branch = ocar(ocdr(true_branch));

    char *true_branch_lab = mk_label();
    char *false_branch_lab = mk_label();
    char *end_branch_lab = mk_label();

    compile_bytecode(cc, c, ocar(cond));
    bs_go_if_nil(cc, false_branch_lab);

    bs_label(cc, true_branch_lab);
    compile_bytecode(cc, c, ocar(true_branch));
    bs_go(cc, end_branch_lab);

    bs_label(cc, false_branch_lab);
    compile_bytecode(cc, c, false_branch);
    bs_label(cc, end_branch_lab);

    free(true_branch_lab);
    free(false_branch_lab);
    free(end_branch_lab);
}

void vm_defmacro(compiled_chunk *cc, context *c, object *o) {
    (void)cc;
    printf("Execing defmacro.\n");
    object *fname = ocar(ocdr(o));
    object *fargs = ocar(ocdr(ocdr(o)));
    object *body = ocdr(ocdr(ocdr(o)));
    if(otype(fname) != O_SYM) {
        printf("Cannot bind function to: ");
        print_object(fname);
        printf("\n");
        abort();
    }
    if(otype(fargs) != O_CONS && fargs != obj_nil()) {
        printf("Expected args as list, but got: ");
        print_object(fargs);
        printf("\n");
        abort();
    }

    compiled_chunk *macro_cc = compile_fn(c, new_object_fn(fargs, body));
    bind_fn(c, fname, new_object_macro_compiled(macro_cc));
}

void vm_backtick(compiled_chunk *cc, context *c, object *o) {
    switch(otype(o)) {
    case O_CONS:
        if(ocar(o) == vm_s_comma) {
            printf("Got comma. Evaluating: ");
            print_object(ocar(ocdr(o)));
            printf("\n");
            compile_bytecode(cc, c, ocar(ocdr(o)));
        }
        else {
            int num_args = 0;
            for(object *each = o; each != obj_nil(); each = ocdr(each)) {
                vm_backtick(cc, c, ocar(each));
                num_args++;
            }
            bs_push(cc, interns("LIST"));
            bs_call(cc, num_args);
        }
        break;
    default:
        bs_push(cc, o);
    }
}

void vm_for(compiled_chunk *cc, context *c, object *o) {
    (void)c;
    (void)o;
    (void)cc;
    object *for_args = ocar(ocdr(o));
    object *setup = ocar(for_args);
    object *cond = ocar(ocdr(for_args));
    object *step = ocar(ocdr(ocdr(for_args)));
    printf("Setup: ");
    print_object(setup);
    printf("\nCond: ");
    print_object(cond);
    printf("\nStep: ");
    print_object(step);
    printf("\n");
}

static void compile_cons(compiled_chunk *cc, context *c, object *o) {
    object *func = ocar(o);

    if(func == vm_s_quote) {
        bs_push(cc, ocar(ocdr(o)));
    }
    else if(func == vm_s_let) {
        vm_let(cc, c, o);
    }
    else if (func == vm_s_fn) {
        vm_fn(cc, c, o);
    }
    else if(func == vm_s_if) {
        vm_if(cc, c, o);
    }
    else if(func == vm_s_defmacro) {
        printf("Calling defmacro.\n");
        vm_defmacro(cc, c, o);
    }
    else if(func == vm_s_backtick) {
        vm_backtick(cc, c, ocar(ocdr(o)));
    }
    else if(func == vm_s_comma) {
        printf("Error: Comma outside of a backtick.\n");
        abort();
    }
//    else if(func == vm_s_for) {
//        vm_for(cc, c, o);
//    }
    else {

        int num_args = 0;
        for(object *curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(cc, c, ocar(curr));
        }
        bs_push(cc, func);
        bs_call(cc, num_args);
    }
}

void compile_bytecode(compiled_chunk *cc, context *c, object *o) {
    switch(otype(o)) {
    case O_CONS:
        compile_cons(cc, c, o);
        break;
    case O_SYM:
        bs_push(cc, o);
        bs_resolve(cc);
        break;
    case O_STR:
        bs_push(cc, o);
        break;
    case O_NUM:
        bs_push(cc, o);
        break;
    case O_KEYWORD:
        bs_push(cc, o);
        break;
    default:
        printf("Something else: ");
        print_object(o);
        printf("\n");
        abort();
    }
}


object *expand_macros_rec(compiled_chunk *cc, context *c, object *o, size_t rec) {
    if(rec >= 4096) {
        printf("Cannot expand macro. Nesting too deep.\n");
        abort();
    }
    rec++;
    (void)cc;
    if(otype(o) == O_CONS) {
        object *fsym = ocar(o);
        object *func = lookup_fn(c, fsym);
        if(func && otype(func) == O_MACRO_COMPILED) {
            // Don't eval the arguments.
            printf("Expanding: ");
            print_object(o);
            printf("\n");
            int num_args = 0;
            for(object *margs = ocdr(o); margs != obj_nil(); margs = ocdr(margs)) {
                push(ocar(margs));
                num_args++;
            }

            run_vm(c, oval_fn_compiled(func));
            object *exp = pop();
            printf("Expanded: ");
            print_object(exp);
            printf("\n");
            exp = expand_macros_rec(cc, c, exp, rec);
            return exp;
        }
        else {
            for(object *margs = o; margs != obj_nil(); margs = ocdr(margs)) {
                osetcar(margs, expand_macros_rec(cc, c, ocar(margs), rec));
            }
            return o;
        }
    }
    else {
        return o;
    }
}

object *expand_macros(compiled_chunk *cc, context *c, object *o) {
    return expand_macros_rec(cc, c, o, 0);
}

object *vm_eval(context *c, object *o) {

    compiled_chunk *cc = new_compiled_chunk();

    o = expand_macros(cc, c, o);
    printf("Expanded: ");
    print_object(o);
    printf("\n");

    printf("Compiling bytecode\n");
    compile_bytecode(cc, c, o);
    bs_exit(cc);
    printf("Stack length: %ld\n", s_off);

    run_vm(c, cc);
    printf("Stack length: %ld\n", s_off);
    return pop();

    return obj_nil();
}





/** MOVE THE FUNCS BELOW TO A NEW FILE. **/

//info("%p\n", bs);
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
//    printf("PUSH: ");
//    print_object(bs->arg);
//    printf("\n");
    push(bs->arg);
    NEXTI;
pop:
//    printf("POP\n");
    pop();
    NEXTI;
call:
//    printf("CALL (%ld)\n", bs->variance);
    call(c, bs->variance);
    NEXTI;
resolve_sym:
//    printf("RESOLVE_SYM\n");
    resolve(c);
    NEXTI;
bind:
//    printf("BIND\n");
    bind(c);
    NEXTI;
push_lex_context:
//    printf("PUSH_LEX_CONTEXT\n");
    c = push_context(c);
    NEXTI;
pop_lex_context:
//    printf("POP_LEX_CONTEXT\n");
    c = pop_context(c);
    NEXTI;
go:
    target = (size_t)map_get(cc->labels, bs->str);
    //printf("GO (%ld)\n", target);
    bs = cc->bs + target;
    goto *bs->instr;
go_if_nil:
    //target = (size_t)map_get(cc->labels, bs->str);
    //printf("GO_IF_NIL (%ld) ", target);
    if(pop() == obj_nil()) {
        //printf("(jumping to %p)\n", (cc->bs + target)->instr);
        target = (size_t)map_get(cc->labels, bs->str);
        bs = cc->bs + target;
        goto *bs->instr;
    }
    //printf("(not jumping)\n");
    NEXTI;

exit:
    //printf("EXIT\n");
    return NULL;
}

map_t *get_vm_addrs() {
    return (map_t *)___vm(NULL, NULL, 1);
}

void run_vm(context *c, compiled_chunk *cc) {
    ___vm(c, cc, 0);
}
