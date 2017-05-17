#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

struct object {
    enum obj_type type;
    union {
        void *o;
        long num;
    }
};

struct cons {
    object *car;
    object *cdr;
};

object *car(cons *c) {
    return c->car;
}

void setcar(cons *c, object *o) {
    c->car = o;
}

object *cdr(cons *c) {
    return c->cdr;
}

void setcdr(cons *c, object *o) {
    c->cdr = o;
}

void print_cdr(object *o) {
    switch(otype(o)) {
    case O_SYM:
        printf(" . %s", string_ptr((string *)o->o));
        break;
    case O_STR:
        printf(" . \"%s\"", string_ptr((string *)o->o));
        break;
    case O_NUM:
        printf(" . %d", o->num);
        break;
    case O_CONS:
        printf(" ");
        print_cons(o->o);
        break;
    }
}

void print_cons(cons *c) {
    object *o = car(c);
    if(o) {
        print_object(o);
        o = cdr(c);
        if(o) {
            print_cdr(o);
        }
    }
}

void print_list(cons *c) {
    printf("(");
    print_cons(c);
    printf(")");
}

void print_object(object *o) {
    switch(otype(o)) {
    case O_SYM:
        printf("%s", string_ptr((string *)o->o));
        break;
    case O_STR:
        printf("\"%s\"", string_ptr((string *)o->o));
        break;
    case O_NUM:
        printf("%d", o->num);
        break;
    case O_CONS:
        print_list(o->o);
        break;
    }
}

cons *new_cons() {
    cons *c = malloc(sizeof (cons));
    c->car = NULL;
    c->cdr = NULL;
}

enum obj_type otype(object *o) {
    return o->type;
}

object *new_object(enum obj_type t, void *o) {
    object *ob = malloc(sizeof (object));
    ob->type = t;
    ob->o = o;
}



/** Actual parser **/

struct token current;

void get_next_tok() {
    printf("Getting next token.\n");
    if(current.type != NONE) {
        free_token(&current);
    }
    next_token(&current);
}

void tok_match(enum toktype t) {
    if(current.type != t) {
        printf("Failed to match toktype %d\n", t);
        abort();
    }
    free_token(&t);
    current.type = NONE;
}

struct token *currtok() {
    if(current.type == NONE) {
        next_token(&current);
    }
    return &current;
}

object *parse_list() {
//    printf("Parsing List.\n");
    object *o = NULL;
    switch(currtok()->type) {
    case RPAREN:
        printf("Matching RPAREN.\n");
        tok_match(RPAREN);
        return NULL;
        break;
    default:
        printf("Got token: ");
        print_token(currtok());
        printf("Getting next form for CAR.\n");
        o = next_form();
        break;
    }
    cons *c = new_cons(); 
    setcar(c, o);
    printf("Parsing list for CDR.\n");
    object *cdr_obj = parse_list();
    setcdr(c, cdr_obj);
    o = new_object(O_CONS, c);
    return o;
}

object *next_form() {
    if(currtok()->type == NONE) get_next_tok();

    object *o;
    cons *c;
    switch(currtok()->type) {
    case LPAREN:
        printf("Got left paren.\n");
        tok_match(LPAREN);
        return parse_list();
        break;
    case SYM:
        printf("Got a symbol: ");
        print_token(currtok());
        o = new_object(O_SYM, new_string_copy(string_ptr(currtok()->data)));
        get_next_tok();
        return o;
        break;
    case STRING:
        printf("Got a string: ");
        print_token(currtok());
        o = new_object(O_STR, new_string_copy(string_ptr(currtok()->data)));
        get_next_tok();
        return o;
        break;
    case NUM:
        printf("Got a number: ");
        print_token(currtok());
        o = new_object(O_NUM, currtok()->num);
        get_next_tok();
        return o;
        break;
    default:
        printf("Got another token: ");
        print_token(currtok());
        get_next_tok();
        return NULL;
    }
}
