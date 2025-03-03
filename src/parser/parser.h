#ifndef PARSER_H
#define PARSER_H

#include "ast/ast.h"
#include "lexer/lexer.h"

typedef struct s_parser t_parser;

t_parser *parser_new(t_lexer *lexer);
void parser_free(t_parser *parser);
t_ast *parser_parse(t_parser *parser);

#endif
