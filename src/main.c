#include <stdio.h>
#include <stdlib.h>

#include "ast/ast.h"
#include "environment/environment.h"
#include "ft_hashmap.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

int main(void) {
  extern const char **environ;

  const char *line = "(foo && bar) || baz | qux > quux; echo hello";
  puts(line);
  t_lexer *lexer = lexer_new(line);
  t_parser *parser = parser_new(lexer);
  t_ast *ast = parser_parse(parser);
  ast_print(ast);
  t_hashmap *environment = environment_new(environ);
  environment_print(environment);
  // evaluator_evaluate(ast, environment);
  environment_free(environment);
  lexer_free(lexer);
  parser_free(parser);
  ast_free(ast);

  environment_free(environment);

  return EXIT_SUCCESS;
}
