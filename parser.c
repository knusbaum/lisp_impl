#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "map.h"
#include "lisp.h"

struct parser {
    FILE *f;
};

parser *new_parser(char *fname) {
    FILE *f = fopen(fname, "r");
    parser *p = malloc(sizeof (parser));
    p->f = f;
    return p;
}

void destroy_parser(parser *p) {
    fclose(p->f);
    free(p);
}

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

    object *conso = new_object_cons(car_obj, cdr_obj);
    return conso;
}

object *next_form() {
    if(currtok()->type == NONE) get_next_tok();

    object *o;
    switch(currtok()->type) {
    case LPAREN:
        tok_match(LPAREN);
        return parse_list();
        break;
    case SYM:
        o = intern(new_string_copy(string_ptr(currtok()->data)));
        clear_tok();
        return o;
        break;
    case STRING:
        o = new_object(O_STR, new_string_copy(string_ptr(currtok()->data)));
        clear_tok();
        return o;
        break;
    case NUM:
        o = new_object_long(currtok()->num);
        clear_tok();
        return o;
        break;
    case KEYWORD:
        o = intern(new_string_copy(string_ptr(currtok()->data)));
        clear_tok();
        return o;
        break;
    case QUOTE:
        get_next_tok();
        o = next_form();
        o = new_object_cons(o, obj_nil());
        return new_object_cons(intern(new_string_copy("QUOTE")), o);
        break;
    default:
        printf("[parser.c][next_form] Got another token: ");
        print_token(currtok());
        get_next_tok();
        return NULL;
    }
}
