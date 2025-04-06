#ifndef EVALUATOR_INTERNAL_H
#define EVALUATOR_INTERNAL_H

#include <stdbool.h>
#include <unistd.h>

#include "ast/ast.h"
#include "environment/environment.h"

typedef struct s_io_context {
  int in_fd;
  int out_fd;
  bool needs_close_in;
  bool needs_close_out;
} t_io_context;

// Node evaluators
int evaluator_list(t_ast *ast, t_hashmap *environment, t_io_context io);
int evaluator_and_or(t_ast *ast, t_hashmap *environment, t_io_context io);
int evaluator_pipe_sequence(t_ast *ast, t_hashmap *environment,
                            t_io_context io);
int evaluator_subshell(t_ast *ast, t_hashmap *environment, t_io_context io);
int evaluator_simple_command(t_ast *ast, t_hashmap *environment,
                             t_io_context io);

// IO redirection
t_io_context evaluator_apply_io_file(t_ast *io_file, t_io_context io);
void evaluator_close_io(t_io_context *io);

// Command handling
bool evaluator_is_builtin(const char *cmd_name);
int evaluator_execute_builtin(t_ast *ast, t_hashmap *environment,
                              t_io_context io);
int evaluator_execute_external(t_ast *ast, t_hashmap *environment,
                               t_io_context io);

// Helper functions
char **evaluator_build_argv(t_ast *cmd);
char **evaluator_build_envp(t_hashmap *environment);

#endif
