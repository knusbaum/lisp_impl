#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "threaded_vm.h"
#include "compiler.h"

int main(void) {

    context *c = new_context();
    vm_init(c);
    compiler_init();
    while(1) {
        printf("> ");
        object *o = next_form(NULL);
        if(o) {
            object *b = vm_eval(c, o);
            print_object(b);
            printf("\n");
        }
        else {
            printf("GOT NULL.\n");
            break;
        }
    }

    printf("Shutting down.\n");
}
