#include "ast.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ft_ansi.h"
#include "ft_stdlib.h"

static void $ast_print(t_ast *ast, int depth);

t_ast *ast_new(t_ast ast) {
  t_ast *new_ast = ft_expect(malloc(sizeof(t_ast)), __func__);
  *new_ast = ast;
  return new_ast;
}

void ast_free(t_ast *ast) {
  if (!ast) return;

  switch (ast->type) {
    case AST_LIST:
      ast_free(ast->list.left);
      ast_free(ast->list.right);
      break;
    case AST_AND_OR:
      ast_free(ast->and_or.left);
      ast_free(ast->and_or.right);
      break;
    case AST_PIPE_SEQUENCE:
      ast_free(ast->pipe_sequence.left);
      ast_free(ast->pipe_sequence.right);
      break;
    case AST_SUBSHELL:
      ast_free(ast->subshell.and_or);
      break;
    case AST_SIMPLE_COMMAND:
      ast_free(ast->simple_command.cmd_prefix);
      ast_free(ast->simple_command.cmd_suffix);
      break;
    case AST_CMD_PREFIX:
      ast_free(ast->cmd_prefix.io_file);
      ast_free(ast->cmd_prefix.cmd_prefix);
      break;
    case AST_CMD_SUFFIX:
      ast_free(ast->cmd_suffix.io_file);
      ast_free(ast->cmd_suffix.cmd_suffix);
      break;
    case AST_IO_FILE:
      break;
  }

  free(ast);
}

const char *ast_type_to_string(t_ast_type type) {
  return (const char *[]){
      [AST_LIST] = "list",
      [AST_AND_OR] = "and-or",
      [AST_PIPE_SEQUENCE] = "pipe-sequence",
      [AST_SUBSHELL] = "subshell",
      [AST_SIMPLE_COMMAND] = "simple-command",
      [AST_CMD_PREFIX] = "cmd-prefix",
      [AST_CMD_SUFFIX] = "cmd-suffix",
      [AST_IO_FILE] = "io-file",
  }[type];
}

void ast_print(t_ast *ast) { $ast_print(ast, 0); }

static const char *colors[] = {
    "\033[38;2;115;138;5m",  "\033[38;2;165;119;6m",  "\033[38;2;33;118;199m",
    "\033[38;2;198;28;111m", "\033[38;2;37;146;134m", "\033[38;2;189;54;19m",
    "\033[38;2;89;86;186m",
};

static const size_t colors_size = sizeof(colors) / sizeof(colors[0]);

#define $get_color(index) (colors[index % colors_size])

static void $ast_print(t_ast *ast, int depth) {
  if (!ast) return;

  static const int indent_size = 2;

  switch (ast->type) {
    case AST_LIST:
      printf("%*s<%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      $ast_print(ast->list.left, depth + 1);
      $ast_print(ast->list.right, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_AND_OR:
      printf("%*s<%s%s\033[0m " ANSI_CYAN "op" ANSI_RESET "=" ANSI_YELLOW
             "\"%s\"" ANSI_RESET ">\n",
             depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type),
             token_type_to_string(ast->and_or.op->type));
      $ast_print(ast->and_or.left, depth + 1);
      $ast_print(ast->and_or.right, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_PIPE_SEQUENCE:
      printf("%*s<%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      $ast_print(ast->pipe_sequence.left, depth + 1);
      $ast_print(ast->pipe_sequence.right, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_SUBSHELL:
      printf("%*s<%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      $ast_print(ast->subshell.and_or, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_SIMPLE_COMMAND:
      printf("%*s<%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      printf("%*s%s\n", (depth + 1) * indent_size, "",
             ast->simple_command.cmd_name);
      $ast_print(ast->simple_command.cmd_prefix, depth + 1);
      $ast_print(ast->simple_command.cmd_suffix, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_CMD_PREFIX:
      printf("%*s<%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      $ast_print(ast->cmd_prefix.io_file, depth + 1);
      $ast_print(ast->cmd_prefix.cmd_prefix, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_CMD_SUFFIX:
      printf("%*s<%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      $ast_print(ast->cmd_suffix.io_file, depth + 1);
      if (ast->cmd_suffix.word) {
        printf("%*s%s\n", (depth + 1) * indent_size, "", ast->cmd_suffix.word);
      }
      $ast_print(ast->cmd_suffix.cmd_suffix, depth + 1);
      printf("%*s</%s%s\033[0m>\n", depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type));
      break;
    case AST_IO_FILE:
      printf("%*s<%s%s\033[0m " ANSI_CYAN "op" ANSI_RESET "=" ANSI_YELLOW
             "\"%s\"" ANSI_RESET " " ANSI_CYAN "filename" ANSI_RESET
             "=" ANSI_YELLOW "\"%s\"" ANSI_RESET " />\n",
             depth * indent_size, "", $get_color(depth),
             ast_type_to_string(ast->type),
             token_type_to_string(ast->io_file.op->type),
             ast->io_file.filename);
      break;
  }
}
