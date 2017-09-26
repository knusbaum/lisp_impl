#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <setjmp.h>
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "threaded_vm.h"
#include "compiler.h"
#include "gc.h"

extern size_t trap_stack_off;
extern size_t s_off;

int main(void) {
    
    context_stack *cs = context_stack_init();
    gc_init();
    vm_init(cs);
    compiler_init();

    pthread_t gc_thread;
    pthread_create(&gc_thread, NULL, run_gc_loop, cs);
    
    compiled_chunk *cc = repl(cs);
    while(1) {
        //printf("\n\n");
        jmp_buf *jmper = vm_push_trap(cs, obj_nil());
        int ret = setjmp(*jmper);
        //printf("RET IS: %d\n", ret);
        if(ret) {
            //printf("Continuing!\n");
            printf("CAUGHT AN ERROR.\n");
            continue;
        }
        //printf("CC: %p\n", cc);
        printf("Current context: %p\nStack: %ld\nTrap: %ld",
               top_context(cs), s_off, trap_stack_off);
        printf("\n> ");
        run_vm(cs, cc);
        pop_trap();
    }

    printf("Shutting down.\n");
}
