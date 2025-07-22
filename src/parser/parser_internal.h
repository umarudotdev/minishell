#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include <stdbool.h>

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser.h"
#include "token/token.h"

struct s_parser {
  t_lexer *lexer;
  t_token *current_token;
  t_token *peek_token;
  bool has_error;
};

t_ast *parser_parse_list(t_parser *parser);
t_ast *parser_parse_and_or(t_parser *parser);
t_ast *parser_parse_pipe_sequence(t_parser *parser);
t_ast *parser_parse_subshell(t_parser *parser);
t_ast *parser_parse_simple_command(t_parser *parser);
t_ast *parser_parse_cmd_prefix(t_parser *parser);
t_ast *parser_parse_cmd_suffix(t_parser *parser);
t_ast *parser_parse_io_file(t_parser *parser);
void parser_advance(t_parser *parser);
bool parser_is_at(t_parser *parser, t_token_type type);
void parser_error(t_parser *parser);

#endif
