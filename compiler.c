#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "threaded_vm.h"

/** VM **/

object *vm_s_quote;
object *vm_s_backtick;
object *vm_s_comma;
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
    return strcmp((char *)s1, (char *)s2) == 0;
}

void compiler_init() {
    vm_s_quote = interns("QUOTE");
    vm_s_backtick = interns("BACKTICK");
    vm_s_comma = interns("COMMA");
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

void bs_push(compiled_chunk *cc, object *o) {
    printf("%ld@%p bs_push: ", cc->b_off, cc);
    print_object(o);
    printf("\n");
    //if(o == vm_s_if) {
    //    abort();
    //}
    add_binstr_arg(cc, map_get(addrs, "push"), o);
    cc->stacklevel++;
    printf("\t Adding 1 to stacklevel. (%ld)\n", cc->stacklevel);
}

void bs_pop(compiled_chunk *cc) {
    printf("%ld@%p bs_pop\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "pop"), NULL);
    cc->stacklevel--;
    printf("\t Subtracting 1 from stacklevel. (%ld)\n", cc->stacklevel);
}

void bs_push_context(compiled_chunk *cc) {
    printf("%ld@%p bs_push_context\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "push_lex_context"), NULL);
    
}

void bs_pop_context(compiled_chunk *cc) {
    printf("%ld@%p bs_pop_context\n", cc->b_off, cc);
    add_binstr_arg(cc, map_get(addrs, "pop_lex_context"), NULL);
}

void bs_bind(compiled_chunk *cc, context *c, object *sym) {
    object *stackoffsetobj = new_object_stackoffset(cc->stacklevel);
    printf("%ld@%p bs_bind %s to stacklevel %ld: (%p)\n", cc->b_off, cc, string_ptr(oval_symbol(sym)), cc->stacklevel, stackoffsetobj);
    //add_binstr_arg(cc, map_get(addrs, "bind"), NULL);
    bind_var(c, sym, stackoffsetobj);
}

void bs_push_from_stack(compiled_chunk *cc, size_t offset) {
    printf("%ld@%p push_from_stack: %ld\n", cc->b_off, cc, offset);
    add_binstr_offset(cc, map_get(addrs, "push_from_stack"), offset);
    cc->stacklevel++;
    printf("\t Adding 1 to stacklevel (%ld).\n", cc->stacklevel);
}

void bs_resolve(compiled_chunk *cc, context *c, object *sym) {
    printf("%ld@%p bs_resolve: resolving: %s\n", cc->b_off, cc, string_ptr(oval_symbol(sym)));
    //
    object *var_stacklevel = lookup_var(c, sym);
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
    else {
        printf("(%p) Type of sym: %d\n", var_stacklevel, otype(var_stacklevel));
        printf("Value: ");
        print_object(var_stacklevel);
        printf("\n");
        bs_push(cc, sym);
        add_binstr_arg(cc, map_get(addrs, "resolve_sym"), NULL);
    }
}

void bs_call(compiled_chunk *cc, long variance) {
    printf("%ld@%p bs_call (%ld)\n", cc->b_off, cc, variance);
    add_binstr_variance(cc, map_get(addrs, "call"), variance);
    cc->stacklevel -= (variance); // num args + function - return val
    printf("\t Subtracting %ld from stacklevel. (%ld)\n", variance, cc->stacklevel);
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
    printf("\t Subtracting 1 from stacklevel. (%ld)\n", cc->stacklevel);    
}

// Reverse the bind order and handle variadic calls
long fn_bind_params(compiled_chunk *cc, context *c, object *curr, long variance) {
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
        //bs_push(cc, param);
        bs_bind(cc, c, param);
//        printf("Binding %p to %ld in %p\n", param, variance, c);
//        bind_var(c, param, new_object_long(variance));
        curr = ocdr(curr);
        if(curr != obj_nil()) {
            printf("Expected end of list, but have: ");
            print_object(curr);
            printf("\n");
        }
        cc->flags |= CC_FLAG_HAS_REST;
        return variance;
    }

    //bs_push(cc, param);

    bs_bind(cc, c, param);
    cc->stacklevel++;
    printf("\t Adding 1 to stacklevel. (%ld)\n", cc->stacklevel);
    //printf("Binding %p to %ld in %p\n", param, variance, c);
    //bind_var(c, param, new_object_long(variance)); //(void *)variance);
    variance = fn_bind_params(cc, c, ocdr(curr), variance);

    

    


    variance++;

    return variance;
}

void fn_call(compiled_chunk *cc, context *c, object *fn) {
    bs_push_context(cc);
    c = push_context(c); // pushing context for var binding.
    object *fargs = oval_fn_args(fn);
    object *fbody = oval_fn_body(fn);

    cc->variance = fn_bind_params(cc, c, fargs, 0);
    cc->stacklevel--; //= cc->variance;
    
    printf("Compiling body.\n"); 
    object *ret = obj_nil();
    bs_push(cc, ret);
    for(; fbody != obj_nil(); fbody = ocdr(fbody)) {
        bs_pop(cc);
        object *form = ocar(fbody);
        compile_bytecode(cc, c, form);
    }
    printf("END Compiling body.\n");
//    for(int i = 0; i < cc->variance; i++) {
//        bs_pop(cc);
//    }
    c = pop_context(c);
    bs_pop_context(cc);
}

void compile_fn(compiled_chunk *fn_cc, context *c, object *fn) {

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
}

static void vm_let(compiled_chunk *cc, context *c, object *o) {
    bs_push_context(cc);

    object *letlist = ocar(ocdr(o));
    for(object *frm = letlist; frm != obj_nil(); frm = ocdr(frm)) {
        object *assign = ocar(frm);
        object *sym = ocar(assign);
        compile_bytecode(cc, c, ocar(ocdr(assign)));
        //bs_push(cc, sym);
        bs_bind(cc, c, sym);
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

    compiled_chunk *fn_cc = new_compiled_chunk();
    object *fn = new_object_fn_compiled(fn_cc);
    bind_fn(c, fname, fn);
    //printf("Compiling fn %s into cc: %p\n", string_ptr(oval_symbol(fname)), fn_cc);
    compile_fn(fn_cc, c, new_object_fn(fargs, body));
}

void vm_if(compiled_chunk *cc, context *c, object *o) {
    object *cond = ocdr(o);
    object *true_branch = ocdr(cond);
    object *false_branch = ocar(ocdr(true_branch));

    char *false_branch_lab = mk_label();
    char *end_branch_lab = mk_label();

    compile_bytecode(cc, c, ocar(cond));
    bs_go_if_nil(cc, false_branch_lab);

    compile_bytecode(cc, c, ocar(true_branch));
    bs_go(cc, end_branch_lab);

    bs_label(cc, false_branch_lab);
    compile_bytecode(cc, c, false_branch);
    bs_label(cc, end_branch_lab);
}

void vm_defmacro(compiled_chunk *cc, context *c, object *o) {
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

    compiled_chunk *macro_cc = new_compiled_chunk();
    compile_fn(macro_cc, c, new_object_fn(fargs, body));
    bind_fn(c, fname, new_object_macro_compiled(macro_cc));
}

void vm_backtick(compiled_chunk *cc, context *c, object *o) {
    switch(otype(o)) {
    case O_CONS:
        if(ocar(o) == vm_s_comma) {
            compile_bytecode(cc, c, ocar(ocdr(o)));
        }
        else {
            int num_args = 0;
            for(object *each = o; each != obj_nil(); each = ocdr(each)) {
                vm_backtick(cc, c, ocar(each));
                num_args++;
            }
            object *fn = lookup_fn(c, interns("LIST"));
            bs_push(cc, fn);
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
        object *fn = lookup_fn(c, func);
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
                compile_bytecode(cc, c, ocar(curr));
            }

            if(num_args > fn_cc->variance) {
                if(fn_cc->flags & CC_FLAG_HAS_REST) {
                    bs_push(cc, lookup_fn(c, interns("LIST")));
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
            printf("Compiling function call.\n");
            int num_args = 0;
            for(object *curr = ocdr(o); curr != obj_nil(); curr = ocdr(curr)) {
                num_args++;
                compile_bytecode(cc, c, ocar(curr));
            }

            bs_push(cc, fn);
            bs_call(cc, num_args);
        }
    }
}

void compile_bytecode(compiled_chunk *cc, context *c, object *o) {
    //object *stack_offset;
    switch(otype(o)) {
    case O_CONS:
        compile_cons(cc, c, o);
        break;
    case O_SYM:
        //bs_push(cc, o);
        bs_resolve(cc, c, o);
//        stack_offset = lookup_var(c, o);
//        printf("Looking up %p in %p: ", o, c);
//        print_object(stack_offset);
//        printf("\n");
//        
//        if(!stack_offset || stack_offset == obj_nil() || otype(stack_offset) != O_NUM) {
//            bs_push(cc, o);
//            bs_resolve(cc);
//        }
//        else {
//            bs_push_from_stack(cc, oval_long(stack_offset));
//        }
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
                printf("Pushing: ");
                print_object(ocar(margs));
                printf("\n");
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

compiled_chunk *compile_form(context *c, object *o) {
    compiled_chunk *cc = new_compiled_chunk();
    
    o = expand_macros(cc, c, o);
    printf("Expanded: ");
    print_object(o);
    printf("\n");

    compile_bytecode(cc, c, o);
    bs_exit(cc);

    return cc;
}
