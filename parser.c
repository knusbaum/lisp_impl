#include <stdlib.h>
#include "parser.h"
#include "lexer.h"

struct object {
    enum obj_type type;
    union {
        void *o;
        long num;
    };
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
    if(!o) return;
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
    return c;
}

enum obj_type otype(object *o) {
    return o->type;
}

object *new_object(enum obj_type t, void *o) {
    object *ob = malloc(sizeof (object));
    ob->type = t;
    ob->o = o;
    return ob;
}



/** Actual parser **/

struct token current;

void get_next_tok() {
    //printf("[parser.c][get_next_tok] Getting next token.\n");
    if(current.type != NONE) {
        free_token(&current);
    }
    next_token(&current);
    //printf("[parser.c][get_next_tok] Ended up with @ %p: ", &current);
    //print_token(&current);
}

void tok_match(enum toktype t) {
    //printf("[parser.c][tok_match] Matching toktype %s\n", toktype_str(t)); 
    if(current.type != t) {
        printf("Failed to match toktype %d\n", t);
        abort();
    }
    free_token(&current);
    current.type = NONE;
}

struct token *currtok() {
    //printf("[parser.c][currtok] Getting currtok.\n");
    if(current.type == NONE) {
        //printf("[parser.c][currtok] Going to call next_token.\n");
        next_token(&current);
    }
    //printf("[parser.c][currtok] currtok: ");
    //print_token(&current);
    return &current;
}

object *parse_list() {
    //printf("[parser.c][parse_list] Entering parse_list\n");
    object *car_obj = NULL;
    switch(currtok()->type) {
    case RPAREN:
        //printf("[parser.c][parse_list] Found end of list.\n");
        tok_match(RPAREN);
        return NULL;
        break;
    default:
        //printf("[parser.c][parse_list] Getting next form for CAR.\n");
        car_obj = next_form();
        break;
    }
    cons *c = new_cons(); 
    setcar(c, car_obj);
    
    //printf("[parser.c][parse_list] Parsing list for CDR.\n");
    object *cdr_obj = parse_list();
    setcdr(c, cdr_obj);
    
    object *conso = new_object(O_CONS, c);
    return conso;
}

object *next_form() {
    if(currtok()->type == NONE) get_next_tok();

    object *o, o2;
    cons *c;
    cons *cdrcons;
    switch(currtok()->type) {
    case LPAREN:
        //printf("[parser.c][next_form] Got left paren.\n");
        tok_match(LPAREN);
        return parse_list();
        break;
    case SYM:
        //printf("[parser.c][next_form] Got a symbol: ");
        //print_token(currtok());
        o = new_object(O_SYM, new_string_copy(string_ptr(currtok()->data)));
        get_next_tok();
        return o;
        break;
    case STRING:
        //printf("[parser.c][next_form] Got a string: ");
        //print_token(currtok());
        o = new_object(O_STR, new_string_copy(string_ptr(currtok()->data)));
        get_next_tok();
        return o;
        break;
    case NUM:
        //printf("[parser.c][next_form] Got a number: ");
        //print_token(currtok());
        o = new_object(O_NUM, currtok()->num);
        get_next_tok();
        return o;
        break;
    case QUOTE:
        get_next_tok();
        o = next_form();
        c = new_cons();
        setcar(c, new_object(O_SYM, new_string_copy("QUOTE")));
        cdrcons = new_cons();
        setcar(cdrcons, o);
        o = new_object(O_CONS, cdrcons);
        setcdr(c, o);
        o = new_object(O_CONS, c);
        return o;
        break;
    default:
        printf("[parser.c][next_form] Got another token: ");
        print_token(currtok());
        get_next_tok();
        return NULL;
    }
}
