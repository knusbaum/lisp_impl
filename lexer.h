#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

enum toktype {
    NONE = 0,
    LPAREN = 1,
    RPAREN,
    SYM,
    STRING,
    NUM,
    QUOTE,
    DOT,
    END
};

struct token {
    enum toktype type;
    union {
        void *data;
        long num;
    };
};

void next_token(struct token *t);
void free_token(struct token *t);
const char *toktype_str(enum toktype t);
void print_token(struct token *t);

// String mechanics

typedef struct string string;

string *new_string();
string *new_string_copy(const char *c);
void string_append(string *s, char c);
void string_trim_capacity(string *s);
size_t string_len(string *s);
size_t string_cap(string *s);
const char *string_ptr(string *s);
int string_cmp(string *s1, string *s2);
int string_equal(string *s1, string *s2);
void string_free(string *s);

#endif
