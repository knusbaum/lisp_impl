#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "threaded_vm.h"

/** VM **/

object *vm_s_quote;
object *vm_s_backtick;
object *vm_s_comma;
object *vm_s_comma_at;
object *vm_s_let;
object *vm_s_fn;
object *vm_s_if;
object *vm_s_defmacro;
object *vm_s_for;

unsigned int i;

char *mk_label() {
    char *label = malloc(12); // 10 digits + L + \0
    sprintf(label, "L%u", i++);
    return label;
}

static int str_eq(void *s1, void *s2) {
    //printf("RUNNING STRING EQ\n");
    return strcmp((char *)s1, (char *)s2) == 0;
}

void compiler_init() {
    vm_s_quote = interns("QUOTE");
    vm_s_backtick = interns("BACKTICK");
    vm_s_comma = interns("COMMA");
    vm_s_comma_at = interns("COMMA_AT");
    vm_s_let = interns("LET");
    vm_s_fn = interns("FN");
    vm_s_if = interns("IF");
    vm_s_defmacro = interns("DEFMACRO");
    vm_s_for = interns("FOR");
}

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

void add_binstr_offset(compiled_chunk *c, void *instr, size_t offset) {
    if(c->b_off == c->bsize) {
        c->bsize *= 2;
        c->bs = realloc(c->bs, sizeof (struct binstr) * c->bsize);
    }

    struct binstr *target = c->bs + c->b_off;
    target->instr = instr;
    target->offset=offset;
    c->b_off++;
}

void free_compiled_chunk(compiled_chunk *c) {
    free(c->bs);
    map_destroy(c->labels);
    free(c);
}

void bs_chew_top(compiled_chunk *cc, size_t num) {
    printf("%ld@%p bs_chew_top: (%ld)", cc->b_off, cc, num);
    add_binstr_offset(cc, map_get(addrs, "chew_top"), num);
    cc->stacklevel -= num;
}

void bs_push(compiled_chunk *cc, object *o) {
    printf("%ld@%p bs_push: ", cc->b_off, cc);
    print_object(o);
    printf("\n");

    add_binstr_arg(cc, map_get(addrs, "push"), o);
    cc->stacklevel++;
}

void bs_pop(compiled_chunk *cc) {
    printf("%ld@%p bs_pop\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "pop"), NULL);
    cc->stacklevel--;
}

void bs_push_context(compiled_chunk *cc) {
    printf("%ld@%p bs_push_context\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "push_lex_context"), NULL);

}

void bs_pop_context(compiled_chunk *cc) {
    printf("%ld@%p bs_pop_context\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "pop_lex_context"), NULL);
}

void bs_bind(compiled_chunk *cc, context_stack *cs, object *sym) {
    object *stackoffsetobj = new_object_stackoffset(cc->stacklevel);
    printf("%ld@%p bs_bind %s to stacklevel %ld: (%p)\n", cc->b_off, cc, string_ptr(oval_symbol(sym)), cc->stacklevel, stackoffsetobj);
    bind_var(cs, sym, stackoffsetobj);
}

void bs_bind_var(compiled_chunk *cc) {
    printf("%ld@%p bs_bind_var\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "bind_var"), NULL);
    cc->stacklevel-=2;
}

void bs_bind_fn(compiled_chunk *cc) {
    printf("%ld@%p bs_bind_fn\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "bind_fn"), NULL);
    cc->stacklevel-=2;
}

void bs_push_from_stack(compiled_chunk *cc, size_t offset) {
    printf("%ld@%p push_from_stack: %ld\n", cc->b_off, cc, offset);
    add_binstr_offset(cc, map_get(addrs, "push_from_stack"), offset);
    cc->stacklevel++;
}

void bs_resolve(compiled_chunk *cc, context_stack *cs, object *sym) {
    object *var_stacklevel = lookup_var(cs, sym);
    if(!var_stacklevel) {
        printf("Cannot resolve sym: ");
        print_object(sym);
        printf("\n");
        abort();
    }
    if(otype(var_stacklevel) == O_STACKOFFSET) {

        printf("Resolving to: %ld, %ld from top of stack (%ld): (%p)\n", oval_stackoffset(var_stacklevel), cc->stacklevel - oval_stackoffset(var_stacklevel), cc->stacklevel, var_stacklevel);
        bs_push_from_stack(cc, cc->stacklevel - oval_stackoffset(var_stacklevel));
    }
    else if(map_get(special_syms, sym) != NULL) {
        bs_push(cc, sym);
    }
    else {
        bs_push(cc, sym);
        printf("%ld@%p bs_resolve: resolving: %s\n", cc->b_off, cc, string_ptr(oval_symbol(sym)));
        add_binstr_arg(cc, map_get(addrs, "resolve_sym"), NULL);
    }
}

void bs_add(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_add (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "add"), variance);
    cc->stacklevel -= ((variance)-1);
}

void bs_subtract(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_subtract (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "subtract"), variance);
    cc->stacklevel -= ((variance)-1);
}
void bs_multiply(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_multiply (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "multiply"), variance);
    cc->stacklevel -= ((variance)-1);
}
void bs_divide(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_divide (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "divide"), variance);
    cc->stacklevel -= ((variance)-1);
}
void bs_num_eq(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_num_eq (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "num_eq"), variance);
    cc->stacklevel -= ((variance)-1);
}

void bs_call(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_call (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "call"), variance);
    cc->stacklevel -= (variance);
}

void bs_exit(compiled_chunk *cc) {
    printf("%ld@%p bs_exit\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "exit"), NULL);
}

void bs_label(compiled_chunk *cc, char *label) {
    printf("%ld@%p bs_label: %s\n", cc->b_off, cc, label);
    map_put(cc->labels, label, (void *)cc->b_off);
}

void bs_go(compiled_chunk *cc, char *label) {
    printf("%ld@%p bs_go: %s\n", cc->b_off, cc, label);
    add_binstr_str(cc, map_get(addrs, "go"), label);
}

void bs_go_if_nil(compiled_chunk *cc, char *label) {
    printf("%ld@%p bs_go_if_nil: %s\n", cc->b_off, cc, label);
    add_binstr_str(cc, map_get(addrs, "go_if_nil"), label);
    cc->stacklevel--;
}

// Reverse the bind order and handle variadic calls
long fn_bind_params(compiled_chunk *cc, context_stack *cs, object *curr, long variance) {
    if(curr == obj_nil()) return variance;

    object *rest = interns("&REST"); // Refactor. Shouldn't be calling interns here.

    object *param = ocar(curr);
    if(param == rest) {
        // We have a variadic function. Mark it and handle further args.
        curr = ocdr(curr);
        param = ocar(curr);
        if(curr == obj_nil() || param == obj_nil()) {
            printf("Must supply a symbol to bind for &REST.\n");
            abort();
        }
        bs_bind(cc, cs, param);
        cc->stacklevel++;
        curr = ocdr(curr);
        if(curr != obj_nil()) {
            printf("Expected end of list, but have: ");
            print_object(curr);
            printf("\n");
        }
        cc->flags |= CC_FLAG_HAS_REST;
        return variance;
    }

    bs_bind(cc, cs, param);
    cc->stacklevel++;
    variance = fn_bind_params(cc, cs, ocdr(curr), variance);
    variance++;

    return variance;
}

void fn_call(compiled_chunk *cc, context_stack *cs, object *fn) {
    push_context(cs); // pushing context for var binding.
    object *fargs = oval_fn_args(fn);
    object *fbody = oval_fn_body(fn);

    cc->variance = fn_bind_params(cc, cs, fargs, 0);
    cc->stacklevel--;

    int first_form = 1;
    for(; fbody != obj_nil(); fbody = ocdr(fbody)) {
        if(!first_form) {
            bs_pop(cc);
        }
        first_form = 0;
        object *form = ocar(fbody);
        compile_bytecode(cc, cs, form);
    }
    pop_context(cs);
}

void compile_fn(compiled_chunk *fn_cc, context_stack *cs, object *fn) {

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
        fn_call(fn_cc, cs, fn);
    }
    else if(otype(fn) == O_MACRO) {
        fn_call(fn_cc, cs, fn);
        object *result = pop();
        compile_bytecode(fn_cc, cs, result);
    }
    bs_exit(fn_cc);
}

static void vm_let(compiled_chunk *cc, context_stack *cs, object *o) {
    object *letlist = ocar(ocdr(o));
    long pushlen = 0;
    for(object *frm = letlist; frm != obj_nil(); frm = ocdr(frm)) {
        object *assign = ocar(frm);
        object *sym = ocar(assign);
        compile_bytecode(cc, cs, ocar(ocdr(assign)));
        bs_bind(cc, cs, sym);
        pushlen++;
    }
    int first_time = 1;
    for(object *body = ocdr(ocdr(o)); body != obj_nil(); body = ocdr(body)) {
        if(!first_time) {
            // Pop the previous result. It will be unused. (only the last form is "returned.")
            bs_pop(cc);
        }
        first_time=0;
        compile_bytecode(cc, cs, ocar(body));

    }
    bs_chew_top(cc, pushlen);
}

static void bind_vars_for_closure(compiled_chunk *cc, context_stack *cs) {
    (void)cc;
    context_var_iterator *it = iterate_vars(cs);
    context_var_iterator *current = it;
    while(current) {
        struct sym_val_pair svp = context_var_iterator_values(current);
        printf("COMPILER Found binding from: ");
        print_object(svp.sym);
        printf("(%p) to: ", svp.sym);
        print_object(svp.val);
        printf("(%p)\n", svp.val);
        current = context_var_iterator_next(current);
        if(otype(svp.val) == O_STACKOFFSET) {
            printf("Binding: ");
            print_object(svp.sym);
            printf("(%p) to: ", svp.sym);
            print_object(svp.val);
            printf("(%p)\n", svp.val);
            bs_resolve(cc, cs, svp.sym);
            bs_push(cc, svp.sym);
            bs_bind_var(cc);
        }
    }
    if(it) {
        destroy_context_var_iterator(it);
    }
}

static void vm_fn(compiled_chunk *cc, context_stack *cs, object *o) {
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

    bs_push_context(cc);
    bind_vars_for_closure(cc, cs);
    bs_push(cc, new_object_fn(fargs, body));
    bs_push(cc, fname);
    bs_push(cc, lookup_fn(cs, interns("COMPILE-FN")));
    bs_call(cc, 2);
    bs_pop_context(cc);
}

void vm_if(compiled_chunk *cc, context_stack *cs, object *o) {
    object *cond = ocdr(o);
    object *true_branch = ocdr(cond);
    object *false_branch = ocar(ocdr(true_branch));

    char *false_branch_lab = mk_label();
    char *end_branch_lab = mk_label();

    // Conditional
    compile_bytecode(cc, cs, ocar(cond));
    bs_go_if_nil(cc, false_branch_lab);
    long stackoff = cc->stacklevel;

    // True branch
    compile_bytecode(cc, cs, ocar(true_branch));
    bs_go(cc, end_branch_lab);

    // False branch
    // stack offset must be reset since above code was not run if we're executing this.
    cc->stacklevel = stackoff;
    bs_label(cc, false_branch_lab);
    compile_bytecode(cc, cs, false_branch);
    bs_label(cc, end_branch_lab);
}

void vm_defmacro(compiled_chunk *cc, context_stack *cs, object *o) {
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

    bs_push_context(cc);
    bind_vars_for_closure(cc, cs);
    bs_push(cc, new_object_fn(fargs, body));
    bs_push(cc, fname);
    bs_push(cc, lookup_fn(cs, interns("COMPILE-MACRO")));
    bs_call(cc, 2);
    bs_pop_context(cc);
}

int vm_backtick(compiled_chunk *cc, context_stack *cs, object *o) {
    switch(otype(o)) {
    case O_CONS:
        if(ocar(o) == vm_s_comma) {
            compile_bytecode(cc, cs, ocar(ocdr(o)));
            return 0;
        }
        else if(ocar(o) == vm_s_comma_at) {
            compile_bytecode(cc, cs, ocar(ocdr(o)));
            return 1;
        }
        else {
            bs_push(cc, obj_nil());
            for(object *each = o; each != obj_nil(); each = ocdr(each)) {
                int should_splice = vm_backtick(cc, cs, ocar(each));
                (void)should_splice;
                if(should_splice) {
                    bs_push(cc, lookup_fn(cs, interns("SPLICE")));
                    bs_call(cc, 2);

                }
                else {
                    bs_push(cc, lookup_fn(cs, interns("APPEND")));
                    bs_call(cc, 2);
                }
            }
            return 0;
        }
        break;
    default:
        bs_push(cc, o);
        return 0;
    }
}

void vm_for(compiled_chunk *cc, context_stack *cs, object *o) {
    (void)cs;
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

static void compile_cons(compiled_chunk *cc, context_stack *cs, object *o) {
    object *func = ocar(o);

    if(func == vm_s_quote) {
        bs_push(cc, ocar(ocdr(o)));
    }
    else if(func == vm_s_let) {
        vm_let(cc, cs, o);
    }
    else if (func == vm_s_fn) {
        vm_fn(cc, cs, o);
    }
    else if(func == vm_s_if) {
        vm_if(cc, cs, o);
    }
    else if(func == vm_s_defmacro) {
        vm_defmacro(cc, cs, o);
    }
    else if(func == vm_s_backtick) {
        vm_backtick(cc, cs, ocar(ocdr(o)));
    }
    else if(func == vm_s_comma) {
        printf("Error: Comma outside of a backtick.\n");
        abort();
    }
    // inlined arithmetic
    else if(func == interns("+")) {
        long num_args = 0;
        object *curr;
        for(curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(cc, cs, ocar(curr));
        }

        bs_add(cc, num_args);
    }
    else if(func == interns("-")) {
        long num_args = 0;
        object *curr;
        for(curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(cc, cs, ocar(curr));
        }

        bs_subtract(cc, num_args);
    }
    else if(func == interns("*")) {
        long num_args = 0;
        object *curr;
        for(curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(cc, cs, ocar(curr));
        }

        bs_multiply(cc, num_args);
    }
    else if(func == interns("/")) {
        long num_args = 0;
        object *curr;
        for(curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(cc, cs, ocar(curr));
        }

        bs_divide(cc, num_args);
    }
    else if(func == interns("=")) {
        long num_args = 0;
        object *curr;
        for(curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
            num_args++;
            compile_bytecode(cc, cs, ocar(curr));
        }

        bs_num_eq(cc, num_args);
    }
    else {
        object *fn = lookup_fn(cs, func);
        if(!fn) {
            printf("No such function: %s\n", string_ptr(oval_symbol(func)));
            abort();
        }

        if(otype(fn) == O_FN_COMPILED) {
            compiled_chunk *fn_cc = oval_fn_compiled(fn);

            long num_args = 0;
            object *curr;
            for(curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
                num_args++;
                compile_bytecode(cc, cs, ocar(curr));
            }

            if(num_args > fn_cc->variance) {
                if(fn_cc->flags & CC_FLAG_HAS_REST) {
                    printf("PUSHING REST AS LIST!!!\n");
                    bs_push(cc, lookup_fn(cs, interns("LIST")));
                    bs_call(cc, num_args - fn_cc->variance);
                }
                else {
                    printf("Expected exactly %ld arguments, but got %ld.\n", fn_cc->variance, num_args);
                    abort();
                }
            }

            bs_push(cc, fn);
            bs_call(cc, num_args);
        }
        else {
            int num_args = 0;
            for(object *curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
                num_args++;
                compile_bytecode(cc, cs, ocar(curr));
            }

            bs_push(cc, fn);
            bs_call(cc, num_args);
        }
    }
}

void compile_bytecode(compiled_chunk *cc, context_stack *cs, object *o) {
    switch(otype(o)) {
    case O_CONS:
        compile_cons(cc, cs, o);
        break;
    case O_SYM:
        bs_resolve(cc, cs, o);
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

compiled_chunk *compile_form(compiled_chunk *cc, context_stack *cs, object *o) {
    compile_bytecode(cc, cs, o);
    bs_exit(cc);
    return cc;
}

compiled_chunk *repl(context_stack *cs) {
    compiled_chunk *cc = new_compiled_chunk();
    bs_push(cc, lookup_fn(cs, interns("READ")));
    bs_call(cc, 0);
    bs_push(cc, lookup_fn(cs, interns("EVAL")));
    bs_call(cc, 1);
    bs_push(cc, lookup_fn(cs, interns("PRINT")));
    bs_call(cc, 1);
    bs_exit(cc);
    return cc;
}
