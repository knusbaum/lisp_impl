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
static int look() {
    //printf("[lexer.c][look] Looking.\n");
    if(!_look) {
        _look = getchar();
    }
    return _look;
}

static int get_char() {
    //printf("[lexer.c][get_char] Getting char.\n");
    if(!_look) {
        _look = getchar();
    }
    int ret = _look;
    _look = getchar();
    //printf("[lexer.c][get_char] Got: %c\n", ret);
    return ret;
}

static void skip_whitespace() {
    //printf("[lexer.c][next_token] Skipping whitespace.\n");
    while(is_whitespace(look())) {
        get_char();
    }
}

static void match(int c) {
    //printf("[lexer.c][match] Matching %c.\n", c);
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
        if(c >=97 && c <= 122) {
            c -= 32;
        }
        string_append(s, c);
        get_char();
    }
    return s;
}

long parse_long() {
    string *s = new_string();
    while(isdigit(look())) {
        string_append(s, look());
        get_char();
    }
    return strtol(string_ptr(s), NULL, 0);
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
        break;
    default:
        if(isalpha(look())) {
            t->type = SYM;
            t->data = parse_symbol();
        }
        else if(isdigit(look())) {
            t->type = NUM;
            t->num = parse_long();
//            get_char();
        }
        else {
            //printf("[lexer.c][next_token] Got invalid character: %c\n", look());
            abort();
        }
    }
    //printf("[lexer.c][next_token] Made token @ %p: ", t);
    //print_token(t);
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
    case END:
    case NUM:
    case NONE:
        break;
    }
}

const char *toktype_str(enum toktype t) {
    switch(t) {
    case NONE:
        return "NONE";
        break;
    case LPAREN:
        return "LPAREN";
        break;
    case RPAREN:
        return "RPAREN";
        break;
    case SYM:
        return "SYM";
        break;
    case STRING:
        return "STRING";
        break;
    case NUM:
        return "NUM";
        break;
    case QUOTE:
        return "QUOTE";
        break;
    case END:
        return "END";
        break;
    default:
        return "UNKNOWN";
    }
}

void print_token(struct token *t) {
    printf("[");
    switch(t->type) {
    case NONE:
    case LPAREN:
    case RPAREN:
    case QUOTE:
    case END:
        printf("%s", toktype_str(t->type));
        break;
    case SYM:
        printf("%s, %s", toktype_str(t->type), string_ptr(t->data));
        break;
    case STRING:
        printf("%s, \"%s\"", toktype_str(t->type), string_ptr(t->data));
        break;
    case NUM:
        printf("%s, %d", toktype_str(t->type), t->num);
        break;
    default:
        printf("UNKNOWN: %d", t->type);
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
    s->len = 0;
    return s;
}

string *new_string_copy(char *c) {
    struct string *s = malloc(sizeof (struct string));
    size_t size = strlen(c) + 1;
    s->s = malloc(size);
    strcpy(s->s, c);
    s->cap = size;
    s->len = size;
    return s;
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
