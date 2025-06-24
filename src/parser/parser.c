#include "parser.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast/ast.h"
#include "ft_stdlib.h"
#include "ft_string.h"
#include "minishell.h"
#include "parser_internal.h"
#include "token/token.h"

t_parser *parser_new(t_lexer *lexer) {
  t_parser *parser = ft_expect(malloc(sizeof(t_parser)), __func__);
  *parser = (t_parser){
      .lexer = lexer,
      .current_token = NULL,
      .peek_token = NULL,
  };
  parser_advance(parser);
  parser_advance(parser);
  return parser;
}

void parser_free(t_parser *parser) { free(parser); }

t_ast *parser_parse(t_parser *parser) { return parser_parse_list(parser); }

t_ast *parser_parse_list(t_parser *parser) {
  t_ast *left = parser_parse_and_or(parser);
  if (!parser_is_at(parser, 1 << TOKEN_SEMI)) {
    return left;
  }

  parser_advance(parser);
  if (parser_is_at(parser, (1 << TOKEN_SEMI) | (1 << TOKEN_AND_IF) |
                               (1 << TOKEN_OR_IF) | (1 << TOKEN_PIPE))) {
    parser_error(parser);
    return NULL;
  }

  t_ast *right = parser_parse_list(parser);

  return ast_new((t_ast){
      AST_LIST,
      .list.left = left,
      .list.right = right,
  });
}

t_ast *parser_parse_and_or(t_parser *parser) {
  t_ast *left = parser_parse_pipe_sequence(parser);
  if (!parser_is_at(parser, (1 << TOKEN_AND_IF) | (1 << TOKEN_OR_IF))) {
    return left;
  }

  t_token *op = parser->current_token;
  parser_advance(parser);
  if (parser_is_at(parser, (1 << TOKEN_AND_IF) | (1 << TOKEN_OR_IF) |
                               (1 << TOKEN_PIPE))) {
    parser_error(parser);
    return NULL;
  }
  t_ast *right = parser_parse_and_or(parser);

  return ast_new((t_ast){
      AST_AND_OR,
      .and_or.left = left,
      .and_or.op = op,
      .and_or.right = right,
  });
}

t_ast *parser_parse_pipe_sequence(t_parser *parser) {
  t_ast *left = parser_parse_simple_command(parser);
  if (!parser_is_at(parser, 1 << TOKEN_PIPE)) {
    return left;
  }

  parser_advance(parser);
  if (parser_is_at(parser, (1 << TOKEN_AND_IF) | (1 << TOKEN_OR_IF) |
                               (1 << TOKEN_PIPE))) {
    parser_error(parser);
    return NULL;
  }

  t_ast *right = parser_parse_pipe_sequence(parser);

  return ast_new((t_ast){
      AST_PIPE_SEQUENCE,
      .pipe_sequence.left = left,
      .pipe_sequence.right = right,
  });
}

t_ast *parser_parse_subshell(t_parser *parser) {
  t_ast *subshell = parser_parse_and_or(parser);
  if (!parser_is_at(parser, 1 << TOKEN_RPAREN)) {
    parser_error(parser);
    return NULL;
  }

  parser_advance(parser);
  if (parser_is_at(parser, (1 << TOKEN_LPAREN) | (1 << TOKEN_RPAREN) |
                               (1 << TOKEN_WORD))) {
    parser_error(parser);
    return NULL;
  }

  return ast_new((t_ast){
      AST_SUBSHELL,
      .subshell.and_or = subshell,
  });
}

t_ast *parser_parse_simple_command(t_parser *parser) {
  if (parser_is_at(parser, 1 << TOKEN_LPAREN)) {
    parser_advance(parser);
    return parser_parse_subshell(parser);
  }

  t_ast *cmd_prefix = parser_parse_cmd_prefix(parser);
  t_string cmd_name = (t_string)parser->current_token->literal;
  parser_advance(parser);
  t_ast *cmd_suffix = parser_parse_cmd_suffix(parser);

  return ast_new((t_ast){
      AST_SIMPLE_COMMAND,
      .simple_command.cmd_prefix = cmd_prefix,
      .simple_command.cmd_name = cmd_name,
      .simple_command.cmd_suffix = cmd_suffix,
  });
}

t_ast *parser_parse_cmd_prefix(t_parser *parser) {
  t_ast *io_file = parser_parse_io_file(parser);
  if (!io_file) {
    return NULL;
  }

  t_ast *cmd_prefix = parser_parse_cmd_prefix(parser);
  return ast_new((t_ast){
      AST_CMD_PREFIX,
      .cmd_prefix.io_file = io_file,
      .cmd_prefix.cmd_prefix = cmd_prefix,
  });
}

t_ast *parser_parse_cmd_suffix(t_parser *parser) {
  t_ast *io_file = parser_parse_io_file(parser);
  if (io_file) {
    t_ast *cmd_suffix = parser_parse_cmd_suffix(parser);
    return ast_new((t_ast){
        AST_CMD_SUFFIX,
        .cmd_suffix.io_file = io_file,
        .cmd_suffix.word = NULL,
        .cmd_suffix.cmd_suffix = cmd_suffix,
    });
  }

  if (!parser_is_at(parser, 1 << TOKEN_WORD)) {
    return NULL;
  }

  const char *word = parser->current_token->literal;
  parser_advance(parser);
  t_ast *cmd_suffix = parser_parse_cmd_suffix(parser);

  return ast_new((t_ast){
      AST_CMD_SUFFIX,
      .cmd_suffix.io_file = NULL,
      .cmd_suffix.word = word,
      .cmd_suffix.cmd_suffix = cmd_suffix,
  });
}

t_ast *parser_parse_io_file(t_parser *parser) {
  if (!parser_is_at(parser, (1 << TOKEN_LESS) | (1 << TOKEN_GREAT) |
                                (1 << TOKEN_DLESS) | (1 << TOKEN_DGREAT))) {
    return NULL;
  }

  t_token *op = parser->current_token;

  parser_advance(parser);
  if (!parser_is_at(parser, 1 << TOKEN_WORD)) {
    parser_error(parser);
    return NULL;
  }

  const char *filename = parser->current_token->literal;
  parser_advance(parser);

  return ast_new((t_ast){
      AST_IO_FILE,
      .io_file.op = op,
      .io_file.filename = filename,
  });
}

void parser_advance(t_parser *parser) {
  parser->current_token = parser->peek_token;
  parser->peek_token = lexer_next_token(parser->lexer);
  token_print(parser->peek_token);
}

bool parser_is_at(t_parser *parser, t_token_type token_type) {
  return (1 << parser->current_token->type) & token_type;
}

void parser_error(t_parser *parser) {
  parser->has_error = true;
  fprintf(stderr, MINISHELL_NAME ": syntax error near unexpected token `%s'\n",
          parser->current_token->literal);
}
