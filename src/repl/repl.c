#include "repl.h"

#include <stdio.h>
//
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ast/ast.h"
#include "environment/environment.h"
#include "evaluator/evaluator.h"
#include "lexer/lexer.h"
#include "minishell.h"
#include "parser/parser.h"
#include "repl_internal.h"

/**
 * @brief Checks if the REPL should continue running based on input.
 *
 * @param line The input line to check.
 * @return true if the REPL should continue, false otherwise.
 */
static bool should_continue_running(const char *line) {
  return line != NULL && strcmp(line, "exit") != 0;
}

/**
 * @brief Processes a line of input.
 *
 * @param input The input string to process.
 * @param environment The environment variables.
 */
static void process_input(const char *input, t_hashmap *environment) {
  t_lexer *lexer = lexer_new(input);
  if (!lexer) {
    fprintf(stderr, "%s: failed to create lexer\n", MINISHELL_NAME);
    return;
  }

  t_parser *parser = parser_new(lexer);
  if (!parser) {
    fprintf(stderr, "%s: failed to create parser\n", MINISHELL_NAME);
    lexer_free(lexer);
    return;
  }

  t_ast *ast = parser_parse(parser);

  if (ast) {
    // Print the AST for debugging purposes
    ast_print(ast);

    // Evaluate the AST
    int status = evaluator_evaluate(ast, environment);

    // Optionally store the status in the environment
    char status_str[16];
    snprintf(status_str, sizeof(status_str), "%d", status);
    char key_value[32];
    snprintf(key_value, sizeof(key_value), "?=%s", status_str);
    environment_set(environment, key_value);

    ast_free(ast);
  }

  parser_free(parser);
  lexer_free(lexer);
}

void repl_start(t_hashmap *environment) {
  char *input;
  char prompt[32];
  bool running = true;

  // Set up signal handlers
  repl_setup_signals();

  // Set up the prompt
  snprintf(prompt, sizeof(prompt), "%s> ", MINISHELL_NAME);

  // Load command history
  read_history(".minishell_history");

  while (running) {
    // Display prompt and get input
    input = readline(prompt);

    // Handle EOF (Ctrl+D)
    if (!input) {
      printf("exit\n");
      break;
    }

    // Add non-empty lines to history
    if (input[0] != '\0') {
      add_history(input);
    }

    // Check if we should continue running
    running = should_continue_running(input);

    if (running && input[0] != '\0') {
      // Process the input
      process_input(input, environment);
    }

    free(input);
  }

  // Save command history
  write_history(".minishell_history");
}
