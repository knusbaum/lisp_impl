#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "lexer.h"
#include "object.h"

typedef struct parser parser;

/** Parser Operations **/
parser *new_parser(char *fname);
void destroy_parser(parser *p);
object *next_form();

#endif
