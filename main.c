#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "lisp.h"

int main(void) {

    context *c = new_context();
    init_context_funcs(c);
    while(1) {
        printf("> ");
        object *o = next_form();
        if(o) {
            object *b = eval(c, o);
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
