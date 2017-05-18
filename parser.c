#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "map.h"
#include "lisp.h"



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

void clear_tok() {
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

void tok_match(enum toktype t) {
    //printf("[parser.c][tok_match] Matching toktype %s\n", toktype_str(t));
    if(currtok()->type != t) {
        printf("Failed to match toktype %s: got: ", toktype_str(t));
        print_token(currtok());
        abort();
    }
    clear_tok();
}

object *parse_list() {
    //printf("[parser.c][parse_list] Entering parse_list\n");
    object *car_obj = NULL;
    switch(currtok()->type) {
    case RPAREN:
        //printf("[parser.c][parse_list] Found end of list.\n");
        tok_match(RPAREN);
        //return NULL;
        return obj_nil();
        break;
    default:
        //printf("[parser.c][parse_list] Getting next form for CAR.\n");
        car_obj = next_form();
        break;
    }
    cons *c = new_cons();
    setcar(c, car_obj);

    object *cdr_obj;
    switch(currtok()->type) {
    case DOT:
        tok_match(DOT);
        cdr_obj = next_form();
        tok_match(RPAREN);
        break;
    default:
        cdr_obj = parse_list();
        break;
    }
    //printf("[parser.c][parse_list] Parsing list for CDR.\n");
    //object *cdr_obj = parse_list();
    setcdr(c, cdr_obj);

    object *conso = new_object(O_CONS, c);
    return conso;
}

object *next_form() {
    if(currtok()->type == NONE) get_next_tok();

    object *o;
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
        o = intern(new_string_copy(string_ptr(currtok()->data)));
        //get_next_tok();
        clear_tok();
        return o;
        break;
    case STRING:
        //printf("[parser.c][next_form] Got a string: ");
        //print_token(currtok());
        o = new_object(O_STR, new_string_copy(string_ptr(currtok()->data)));
        //get_next_tok();
        clear_tok();
        return o;
        break;
    case NUM:
        //printf("[parser.c][next_form] Got a number: ");
        //print_token(currtok());
        o = new_object_long(currtok()->num);
        //get_next_tok();
        clear_tok();
        return o;
        break;
    case QUOTE:
        get_next_tok();
        o = next_form();
        c = new_cons();
        setcar(c, intern(new_string_copy("QUOTE")));
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
