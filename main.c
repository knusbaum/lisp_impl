#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "lisp.h"
#include "threaded_vm.h"


int main(void) {

    context *c = new_context();
    vm_init(c);
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
