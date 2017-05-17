#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main(void) {
    string *s = new_string();
    string_append(s, 'a');
    string_append(s, 'b');
    string_append(s, 'c');
    string_append(s, 'd');
    string_append(s, 'e');
    string_append(s, 'f');
    string_append(s, 'g');
    string_append(s, 'h');
    string_append(s, 'i');
    string_append(s, 'j');
    string_append(s, 'k');
    string_append(s, 'l');
    string_append(s, 'm');
    string_append(s, 'n');
    string_append(s, 'o');
    string_append(s, 'p');
    string_append(s, 'q');
    string_append(s, 'r');
    string_append(s, 's');
    string_append(s, 't');
    string_append(s, 'u');
    string_append(s, 'v');
    string_append(s, 'w');
    string_append(s, 'x');
    string_append(s, 'y');
    string_append(s, 'z');
    printf("String: [%s]\n", string_ptr(s));
    printf("String len: %d\n", string_len(s));
    printf("String cap: %d\n", string_cap(s));
    printf("Trimming Capacity.\n");
    string_trim_capacity(s);
    printf("String: [%s]\n", string_ptr(s));
    printf("String len: %d\n", string_len(s));
    printf("String cap: %d\n", string_cap(s));

    while(1) {
//        struct token t;
//        next_token(&t);
//        print_token(&t);
//        if(t.type == END) break;
        object *o = next_form();
        if(o) {
            printf("Got object: ");
            print_object(o);
            printf("\n");
        }
        else {
            printf("GOT NULL.\n");
            break;
        }
    }

    printf("Shutting down.\n");

//    cons *a = new_cons();
//    cons *b = new_cons();
//    cons *c = new_cons();
//    cons *d = new_cons();
//
//    object *conso1 = new_object(O_CONS, a);
//    object *conso2 = new_object(O_CONS, b);
//    object *conso3 = new_object(O_CONS, c);
//    object *conso4 = new_object(O_CONS, d);
//
//    s = new_string_copy("aoeu");
//    object *str1 = new_object(O_STR, s);
//    setcar(a, conso3);
//    setcdr(a, conso2);
//
//    s = new_string_copy("SYM1");
//    object *str2 = new_object(O_SYM, s);
//    setcar(b, str2);
//    setcdr(b, conso3);
//
//    s = new_string_copy("dhnt");
//    object *str3 = new_object(O_STR, s);
//    setcar(c, str3);
//    setcdr(c, conso4);
//
//    s = new_string_copy("DEFUN");
//    object *str4 = new_object(O_SYM, s);
//    setcar(d, str4);
//
//    s = new_string_copy("LAST");
//    object *str5 = new_object(O_SYM, s);
//    setcdr(d, str5);
//
//    print_object(conso1);
//    printf("\n");
}
