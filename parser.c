#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "map.h"
#include "lisp.h"

struct object {
    enum obj_type type;
    union {
        string *str;
        cons *c;
        long num;
        object *(*native)(void *, void *);
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

void print_cons(cons *c);

void print_cdr(object *o) {
    switch(otype(o)) {
    case O_SYM:
        printf(" . %s", string_ptr(o->str));
        break;
    case O_STR:
        printf(" . \"%s\"", string_ptr(o->str));
        break;
    case O_NUM:
        printf(" . %ld", o->num);
        break;
    case O_CONS:
        printf(" ");
        print_cons(o->c);
        break;
    case O_FN_NATIVE:
        printf(" . #<NATIVE FUNCTION>");
        break;
    }
}

void print_cons(cons *c) {
    object *o = car(c);
    if(o) {
        print_object(o);
        o = cdr(c);
        if(o != obj_nil()) {
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
        printf("%s", string_ptr(o->str));
        break;
    case O_STR:
        printf("\"%s\"", string_ptr(o->str));
        break;
    case O_NUM:
        printf("%ld", o->num);
        break;
    case O_CONS:
        print_list(o->c);
        break;
    case O_FN_NATIVE:
        printf("#<NATIVE FUNCTION>");
        break;
    }
}

map_t *symbols;

object *intern(string *symname) {
    //printf("Interning %s\n", string_ptr(symname));
    if(!symbols) {
        symbols = map_create((int (*)(void *, void *))string_equal);
    }

    object *s = map_get(symbols, symname);
    if(!s) {
        //printf("Wasn't interned yet %s\n", string_ptr(symname));
        s = new_object(O_SYM, symname);
        s->str = symname;
        map_put(symbols, symname, s);
    }
    else {
        //printf("Already interned %s\n", string_ptr(symname));
        string_free(symname);
    }
    return s;
}

object *nil;
object *obj_nil() {
    if(!nil) {
        nil = intern(new_string_copy("NIL"));
    }
    return nil;
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

string *oval_symbol(object *o) {
    if(o->type != O_SYM) {
        printf("Expected sym, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->str;
}

string *oval_string(object *o) {
    if(o->type != O_STR) {
        printf("Expected string, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->str;
}

cons *oval_cons(object *o) {
    if(o->type != O_CONS) {
        printf("Expected cons, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->c;
}

long oval_long(object *o) {
    if(o->type != O_NUM) {
        printf("Expected number, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->num;
}

//void *oval_native(object *o) {
object *(*oval_native(object *o))(void *, void *) {
    if(o->type != O_FN_NATIVE) {
        printf("Expected number, but have: ");
        print_object(o);
        printf("\n");
        abort();
    }
    return o->native;
}

object *ocar(object *o) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
    }
    return car(o->c);
}

object *ocdr(object *o) {
    if(o->type != O_CONS) {
        printf("Expected CONS cell, but have: ");
        print_object(o);
        printf("\n");
    }
    return cdr(o->c);
}

object *new_object(enum obj_type t, void *o) {
    object *ob = malloc(sizeof (object));
    ob->type = t;
    switch(t) {
    case O_SYM:
    case O_STR:
        ob->str = o;
        break;
    case O_NUM:
        printf("Cannot assign a num with this function.\n");
        abort();
        break;
    case O_CONS:
        ob->c = o;
    case O_FN_NATIVE:
        ob->native = o;
        break;
    }
    return ob;
}

object *new_object_long(long l) {
    object *ob = malloc(sizeof (object));
    ob->type = O_NUM;
    ob->num = l;
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
