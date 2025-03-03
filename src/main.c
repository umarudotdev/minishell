#include <stdlib.h>

#include "ast/ast.h"
#include "context/environment.h"
#include "ft_hashmap.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "token/token.h"

int main(void) {
  t_lexer *lexer = lexer_new("echo 42");
  t_parser *parser = parser_new(lexer);
  t_ast *ast = parser_parse(parser);

  ast_print(ast);
  ast_free(ast);
  lexer_free(lexer);
  parser_free(parser);

  // t_hashmap *environment = environment_new();
  // environment_print(environment);
  // environment_free(environment);

  return EXIT_SUCCESS;
}
