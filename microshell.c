#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "minishell.h"

typedef enum e_ast_node_type {
  AST_CD,
  AST_COMMAND,
  AST_PIPELINE,
  AST_SEQUENCE,
} t_ast_node_type;

typedef struct s_ast {
  t_ast_node_type type;
  union {
    struct {
      char *pathname;
    } cd;
    struct {
      int argc;
      char **argv;
    } command;
    struct {
      struct s_ast *left;
      struct s_ast *right;
    } pipeline;
    struct {
      struct s_ast *left;
      struct s_ast *right;
    } sequence;
  };
} t_ast;

size_t $strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    ++len;
  }
  return len;
}

#define $print(...)                                               \
  do {                                                            \
    const char *args[] = {__VA_ARGS__};                           \
    for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); ++i) { \
      write(STDERR_FILENO, args[i], $strlen(args[i]));            \
    }                                                             \
  } while (0)

#define $fatal              \
  do {                      \
    $print("error: fatal"); \
    exit(EXIT_FAILURE);     \
  } while (0)

void *expect(void *ptr) {
  if (!ptr) $fatal;
  return ptr;
}

t_ast *ast_new(t_ast node) {
  t_ast *new = expect(malloc(sizeof(t_ast)));
  *new = node;
  return new;
}

void ast_free(t_ast *node) {
  if (!node) return;

  switch (node->type) {
    case AST_CD:
      break;
    case AST_COMMAND:
      free(node->command.argv);
      break;
    case AST_PIPELINE:
      ast_free(node->pipeline.left);
      ast_free(node->pipeline.right);
      break;
    case AST_SEQUENCE:
      ast_free(node->sequence.left);
      ast_free(node->sequence.right);
      break;
  }

  free(node);
}

int g_argc;
char **g_argv;
int g_index;
t_ast *root;

t_ast *parser_parse_sequence(void);
t_ast *parser_parse_pipeline(void);
t_ast *parser_parse_command(void);

t_ast *parser_parse(int argc, char *argv[]) {
  g_argc = argc;
  g_argv = argv;
  g_index = 1;

  return parser_parse_sequence();
}

bool parser_is_at(const char *token) {
  return strcmp(g_argv[g_index], token) == 0;
}

t_ast *parser_parse_sequence(void) {
  t_ast *node = parser_parse_pipeline();
  while (g_index < g_argc && parser_is_at(";")) {
    ++g_index;

    t_ast *right = parser_parse_pipeline();
    t_ast *sequence = ast_new((t_ast){
        AST_SEQUENCE,
        .sequence.left = node,
        .sequence.right = right,
    });

    node = sequence;
  }
  return node;
}

t_ast *parser_parse_pipeline(void) {
  t_ast *node = parser_parse_command();
  while (g_index < g_argc && parser_is_at("|")) {
    ++g_index;

    t_ast *right = parser_parse_command();
    t_ast *pipeline = ast_new((t_ast){
        AST_PIPELINE,
        .pipeline.left = node,
        .pipeline.right = right,
    });

    node = pipeline;
  }
  return node;
}

t_ast *parser_parse_command(void) {
  if (g_index >= g_argc) return NULL;

  int start = g_index;
  int count = 0;

  while (g_index < g_argc && !parser_is_at(";") && !parser_is_at("|")) {
    ++g_index;
    ++count;
  }
  if (count == 0) return NULL;

  if (strcmp(g_argv[start], "cd") == 0) {
    t_ast *node = ast_new((t_ast){
        AST_CD,
        .cd.pathname = NULL,
    });

    if (count == 2) {
      node->cd.pathname = g_argv[start + 1];
    }

    return node;
  } else {
    t_ast *node = ast_new((t_ast){
        AST_COMMAND,
        .command.argc = count,
        .command.argv = expect(malloc((count + 1) * sizeof(char *))),
    });

    for (int i = 0; i < count; ++i) {
      node->command.argv[i] = g_argv[start + i];
    }
    node->command.argv[count] = NULL;

    return node;
  }
}

int ast_execute(t_ast *node, int in_fd, int out_fd, char *envp[]) {
  int status = EXIT_SUCCESS;

  if (!node) return status;

  switch (node->type) {
    case AST_CD:
      if (node->cd.pathname == NULL) {
        $print("error: cd: bad arguments\n");
        status = EXIT_FAILURE;
      }

      if (chdir(node->cd.pathname) == -1) {
        $print("error: cd: cannot change directory to ", node->cd.pathname,
               "\n");
        status = EXIT_FAILURE;
      }
      break;
    case AST_COMMAND: {
      pid_t pid = fork();
      if (pid == -1) $fatal;

      if (pid == 0) {
        if (dup2(in_fd, 0) == -1) $fatal;
        if (dup2(out_fd, 1) == -1) $fatal;

        execve(node->command.argv[0], node->command.argv, envp);
        $print("error: cannot execute ", node->command.argv[0], "\n");

        ast_free(root);
        exit(EXIT_FAILURE);
      }

      {
        int s;
        waitpid(pid, &s, 0);
        status = WEXITSTATUS(s);
      }
      break;
    }
    case AST_PIPELINE: {
      int fd[2];
      if (pipe(fd)) $fatal;

      pid_t left_pid = fork();
      if (left_pid == -1) $fatal;

      if (left_pid == 0) {
        if (dup2(in_fd, 0) == -1) $fatal;
        if (dup2(fd[1], 1) == -1) $fatal;

        close(fd[0]);
        close(fd[1]);

        int s = ast_execute(node->pipeline.left, 0, 1, envp);
        ast_free(root);
        exit(s);
      }

      pid_t right_pid = fork();
      if (right_pid == -1) $fatal;

      if (right_pid == 0) {
        if (dup2(fd[0], 0) == -1) $fatal;
        if (dup2(out_fd, 1) == -1) $fatal;

        close(fd[0]);
        close(fd[1]);

        int s = ast_execute(node->pipeline.right, 0, 1, envp);
        ast_free(root);
        exit(s);
      }

      close(fd[0]);
      close(fd[1]);

      {
        int s;
        waitpid(left_pid, &s, 0);
        waitpid(right_pid, &s, 0);
        status = WEXITSTATUS(s);
      }
      break;
    }
    case AST_SEQUENCE:
      status = ast_execute(node->sequence.left, in_fd, out_fd, envp);
      status = ast_execute(node->sequence.right, in_fd, out_fd, envp);
      break;
  }
  return status;
}

void ast_print(t_ast *node, int depth);

int main(int argc, char *argv[], char *envp[]) {
  assert(MINISHELL_VERSION_MAJOR == 0);

  root = parser_parse(argc, argv);

  ast_print(root, 0);
  ast_execute(root, 0, 1, envp);
  ast_free(root);

  return EXIT_SUCCESS;
}

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

void ast_print(t_ast *node, int depth) {
  if (!node) return;

  switch (node->type) {
    case AST_CD:
      printf("%*s" GREEN "<cd>" RESET "\n", depth * 2, "");
      printf("%*s" YELLOW "%s" RESET "\n", (depth + 1) * 2, "",
             node->cd.pathname);
      printf("%*s" RED "</cd>" RESET "\n", depth * 2, "");
      break;
    case AST_COMMAND:
      printf("%*s" GREEN "<command>" RESET "\n", depth * 2, "");
      printf("%*s" YELLOW "%s" RESET "\n", (depth + 1) * 2, "",
             node->command.argv[0]);
      for (int i = 1; i < node->command.argc; ++i) {
        printf("%*s%s\n", (depth + 1) * 2, "", node->command.argv[i]);
      }
      printf("%*s" RED "</command>" RESET "\n", depth * 2, "");
      break;
    case AST_PIPELINE:
      printf("%*s" GREEN "<pipeline>" RESET "\n", depth * 2, "");
      ast_print(node->pipeline.left, depth + 1);
      ast_print(node->pipeline.right, depth + 1);
      printf("%*s" RED "</pipeline>" RESET "\n", depth * 2, "");
      break;
    case AST_SEQUENCE:
      printf("%*s" GREEN "<sequence>" RESET "\n", depth * 2, "");
      ast_print(node->sequence.left, depth + 1);
      ast_print(node->sequence.right, depth + 1);
      printf("%*s" RED "</sequence>" RESET "\n", depth * 2, "");
      break;
  }
}
