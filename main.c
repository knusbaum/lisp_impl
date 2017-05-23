#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "threaded_vm.h"
#include "compiler.h"

int main(void) {

    context_stack *cs = context_stack_init();
    vm_init(cs);
    compiler_init();

    compiled_chunk *cc = repl(cs);
    while(1) {
        printf("\n> ");
        run_vm(cs, cc);
    }

    printf("Shutting down.\n");
}
