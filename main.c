#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "threaded_vm.h"
#include "compiler.h"
#include "gc.h"

int main(void) {
    
    context_stack *cs = context_stack_init();
    gc_init();
    vm_init(cs);
    compiler_init();

    //pthread_t gc_thread;
    //pthread_create(&gc_thread, NULL, run_gc_loop, cs);
    
    compiled_chunk *cc = repl(cs);
    while(1) {
        printf("\n> ");
        run_vm(cs, cc);
        //printf("\nDumping stack.\n");
        //dump_stack();
        //gc(cs);
        //dump_heap();
    }

    printf("Shutting down.\n");
}
