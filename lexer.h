#ifndef LEXER_H
#define LEXER_H

enum toktype {
    NONE = 0,
    LPAREN = 1,
    RPAREN,
    SYM,
    KEYWORD,
    STRING,
    NUM,
    QUOTE,
    BACKTICK,
    COMMA,
    AT_SYMBOL,
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

#endif
