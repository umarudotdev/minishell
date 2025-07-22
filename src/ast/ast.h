#ifndef AST_H
#define AST_H

#include "token/token.h"

typedef enum e_ast_type {
  AST_LIST,
  AST_AND_OR,
  AST_PIPE_SEQUENCE,
  AST_SUBSHELL,
  AST_SIMPLE_COMMAND,
  AST_CMD_PREFIX,
  AST_CMD_SUFFIX,
  AST_IO_FILE,
} t_ast_type;

typedef struct s_ast {
  t_ast_type type;
  union {
    struct {
      struct s_ast *left;
      struct s_ast *right;
    } list;
    struct {
      struct s_ast *left;
      t_token *op;
      struct s_ast *right;
    } and_or;
    struct {
      struct s_ast *left;
      struct s_ast *right;
    } pipe_sequence;
    struct {
      struct s_ast *and_or;
    } subshell;
    struct {
      struct s_ast *cmd_prefix;
      const char *cmd_name;
      struct s_ast *cmd_suffix;
    } simple_command;
    struct {
      struct s_ast *io_file;
      struct s_ast *cmd_prefix;
    } cmd_prefix;
    struct {
      struct s_ast *io_file;
      const char *word;
      struct s_ast *cmd_suffix;
    } cmd_suffix;
    struct {
      t_token *op;
      const char *filename;
    } io_file;
  };
} t_ast;

t_ast *ast_new(t_ast ast);
void ast_free(t_ast *ast);
const char *ast_type_to_string(t_ast_type type);
void ast_print(t_ast *ast);

#endif
