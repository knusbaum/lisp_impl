#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"

int is_whitespace(int c) {
    return
        c == ' ' ||
        c == '\n' ||
        c == '\t';
}

int is_sym_char(int c) {
    return
        isalnum(c) ||
        c == '-' ||
        c == '_' ||
        c == '*' ||
        c == '+';
}

int _look;
int look() {
    if(!_look) {
        _look = getchar();
    }
    return _look;
}

int get_char() {
    if(!_look) {
        _look = getchar();
    }
    int ret = _look;
    _look = getchar();
    return ret;
}

void skip_whitespace() {
    while(is_whitespace(look())) {
        get_char();
    }
}

void match(int c) {
    if(look() == c) {
        //get_char();
        _look = 0;
        return;
    }
    printf("BAD MATCH! Wanted: %c, got: %c\n", c, look());
    abort();
}

static void append_escaped(string *s, int c) {
    switch(c) {
    case 'n':
        string_append(s, '\n');
        break;
    case 't':
        string_append(s, '\t');
        break;
    default:
        break;
    }
}

string *parse_string() {
    match('"');
    int c;
    string *s = new_string();
    while((c = look()) != '"') {
        switch(c) {
        case '\\':
            match('\\');
            append_escaped(s, look());
            break;
        default:
            string_append(s, c);
            get_char();
        }
    }
    match('"');
    return s;
}

string *parse_symbol() {
    string *s = new_string();
    int c;
    while(c = look(), is_sym_char(c)) {
        string_append(s, c);
        get_char();
    }
    return s;
}

void next_token(struct token *t) {
    skip_whitespace();

    switch(look()) {
    case '(':
        t->type = LPAREN;
        t->data = NULL;
        get_char();
        break;
    case ')':
        t->type = RPAREN;
        t->data = NULL;
        get_char();
        break;
    case '\'':
        t->type = QUOTE;
        t->data = NULL;
        get_char();
        break;
    case '"':
        t->type = STRING;
        t->data = parse_string();
        break;
    case EOF:
        t->type = END;
        t->data = NULL;
    default:
        if(isalpha(look())) {
            t->type = SYM;
            t->data = parse_symbol();
            printf("Got Symbol: ");
            print_token(t);
        }
        else if(isdigit(look())) {
            t->type = NUM;
            t->num = 0;
            get_char();
        }
        else {
            printf("Got invalid character: %c\n", look());
            abort();
        }
    }
}

void free_token(struct token *t) {
    switch(t->type) {
    case SYM:
        string_free(t->data);
        break;
    case STRING:
        string_free(t->data);
        break;
    case LPAREN:
    case RPAREN:
    case QUOTE:
        break;
    }
}

void print_token(struct token *t) {
    printf("[");
    switch(t->type) {
    case NONE:
        printf("NONE");
        break;
    case LPAREN:
        printf("LPAREN");
        break;
    case RPAREN:
        printf("RPAREN");
        break;
    case SYM:
        printf("SYM, %s", string_ptr(t->data));
        break;
    case STRING:
        printf("STRING, \"%s\"", string_ptr(t->data));
        break;
    case NUM:
        printf("NUM, %d", t->num);
        break;
    case QUOTE:
        printf("QUOTE");
        break;
    case END:
        printf("END");
        break;
    }
    printf("]\n");
}

/** String Machinery **/

#define STR_INITIAL_CAPACITY 8

struct string {
    char *s;
    size_t len;
    size_t cap;
};

string *new_string() {
    struct string *s = malloc(sizeof (struct string));
    s->s = NULL;
    s->cap = 0;
}

string *new_string_copy(char *c) {
    struct string *s = malloc(sizeof (struct string));
    size_t size = strlen(c) + 1;
    s->s = malloc(size);
    strcpy(s->s, c);
    s->cap = size;
    s->len = size;
}

void string_append(string *s, char c) {
    if(!s->s) {
        s->s = malloc(STR_INITIAL_CAPACITY);
        s->cap = STR_INITIAL_CAPACITY;
        s->len = 0;
    }

    if(s->len == s->cap - 1) {
        // time to expand.
        s->cap *= 2;
        s->s = realloc(s->s, s->cap);
    }

    s->s[s->len++] = c;
    s->s[s->len] = 0;
}

void string_trim_capacity(string *s) {
    s->s = realloc(s->s, s->len + 1);
    s->cap = s->len + 1;
}

size_t string_len(string *s) {
    return s->len;
}

size_t string_cap(string *s) {
    return s->cap;
}

const char *string_ptr(string *s) {
    return s->s;
}

void string_free(string *s) {
    free(s->s);
    free(s);
}
