#include "evaluator.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "evaluator_internal.h"
#include "ft_ansi.h"
#include "ft_stdlib.h"
#include "ft_string.h"
#include "minishell.h"

int evaluator_evaluate(t_ast *ast, t_hashmap *environment) {
  t_io_context io = {.in_fd = STDIN_FILENO,
                     .out_fd = STDOUT_FILENO,
                     .needs_close_in = false,
                     .needs_close_out = false};

  if (!ast) return EXIT_SUCCESS;

  int status;

  switch (ast->type) {
    case AST_LIST:
      status = evaluator_list(ast, environment, io);
      break;
    case AST_AND_OR:
      status = evaluator_and_or(ast, environment, io);
      break;
    case AST_PIPE_SEQUENCE:
      status = evaluator_pipe_sequence(ast, environment, io);
      break;
    case AST_SUBSHELL:
      status = evaluator_subshell(ast, environment, io);
      break;
    case AST_SIMPLE_COMMAND:
      status = evaluator_simple_command(ast, environment, io);
      break;
    default:
      fprintf(stderr, "%s: unhandled AST node type: %s\n", MINISHELL_NAME,
              ast_type_to_string(ast->type));
      status = EXIT_FAILURE;
  }

  return status;
}

int evaluator_list(t_ast *ast, t_hashmap *environment, t_io_context io) {
  int status;

  status = evaluator_evaluate(ast->list.left, environment);
  if (ast->list.right) {
    status = evaluator_evaluate(ast->list.right, environment);
  }

  return status;
}

int evaluator_and_or(t_ast *ast, t_hashmap *environment, t_io_context io) {
  int left_status = evaluator_evaluate(ast->and_or.left, environment);

  if (ast->and_or.op->type == TOKEN_AND_IF) {
    // Execute right side only if left side succeeded
    if (left_status == EXIT_SUCCESS) {
      return evaluator_evaluate(ast->and_or.right, environment);
    }
    return left_status;
  } else if (ast->and_or.op->type == TOKEN_OR_IF) {
    // Execute right side only if left side failed
    if (left_status != EXIT_SUCCESS) {
      return evaluator_evaluate(ast->and_or.right, environment);
    }
    return left_status;
  }

  return left_status;
}

int evaluator_pipe_sequence(t_ast *ast, t_hashmap *environment,
                            t_io_context io) {
  int pipe_fds[2];
  pid_t left_pid, right_pid;
  int status = EXIT_SUCCESS;

  if (pipe(pipe_fds) == -1) {
    perror(MINISHELL_NAME);
    return EXIT_FAILURE;
  }

  left_pid = fork();
  if (left_pid == -1) {
    perror(MINISHELL_NAME);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return EXIT_FAILURE;
  }

  if (left_pid == 0) {
    // Child process (left side of pipe)
    close(pipe_fds[0]);
    t_io_context child_io = {.in_fd = io.in_fd,
                             .out_fd = pipe_fds[1],
                             .needs_close_in = io.needs_close_in,
                             .needs_close_out = true};

    int exit_status = evaluator_evaluate(ast->pipe_sequence.left, environment);
    exit(exit_status);
  }

  // Parent continues
  right_pid = fork();
  if (right_pid == -1) {
    perror(MINISHELL_NAME);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return EXIT_FAILURE;
  }

  if (right_pid == 0) {
    // Child process (right side of pipe)
    close(pipe_fds[1]);
    t_io_context child_io = {.in_fd = pipe_fds[0],
                             .out_fd = io.out_fd,
                             .needs_close_in = true,
                             .needs_close_out = io.needs_close_out};

    int exit_status = evaluator_evaluate(ast->pipe_sequence.right, environment);
    exit(exit_status);
  }

  // Parent closes both pipe ends
  close(pipe_fds[0]);
  close(pipe_fds[1]);

  // Wait for both children
  int status_left, status_right;
  waitpid(left_pid, &status_left, 0);
  waitpid(right_pid, &status_right, 0);

  // Return status of the right command
  if (WIFEXITED(status_right)) {
    return WEXITSTATUS(status_right);
  }

  return EXIT_FAILURE;
}

int evaluator_subshell(t_ast *ast, t_hashmap *environment, t_io_context io) {
  pid_t pid = fork();

  if (pid == -1) {
    perror(MINISHELL_NAME);
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    // Child process
    if (io.needs_close_in && dup2(io.in_fd, STDIN_FILENO) == -1) {
      perror(MINISHELL_NAME);
      exit(EXIT_FAILURE);
    }

    if (io.needs_close_out && dup2(io.out_fd, STDOUT_FILENO) == -1) {
      perror(MINISHELL_NAME);
      exit(EXIT_FAILURE);
    }

    // Close fds if needed
    evaluator_close_io(&io);

    int exit_status = evaluator_evaluate(ast->subshell.and_or, environment);
    exit(exit_status);
  }

  // Parent process
  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  return EXIT_FAILURE;
}

int evaluator_simple_command(t_ast *ast, t_hashmap *environment,
                             t_io_context io) {
  // Apply any IO redirections from cmd_prefix
  if (ast->simple_command.cmd_prefix) {
    t_ast *prefix = ast->simple_command.cmd_prefix;
    while (prefix) {
      if (prefix->cmd_prefix.io_file) {
        io = evaluator_apply_io_file(prefix->cmd_prefix.io_file, io);
      }
      prefix = prefix->cmd_prefix.cmd_prefix;
    }
  }

  // Apply any IO redirections from cmd_suffix
  if (ast->simple_command.cmd_suffix) {
    t_ast *suffix = ast->simple_command.cmd_suffix;
    while (suffix) {
      if (suffix->cmd_suffix.io_file) {
        io = evaluator_apply_io_file(suffix->cmd_suffix.io_file, io);
      }
      suffix = suffix->cmd_suffix.cmd_suffix;
    }
  }

  // Check for builtin commands
  if (evaluator_is_builtin(ast->simple_command.cmd_name)) {
    return evaluator_execute_builtin(ast, environment, io);
  }

  // External command
  return evaluator_execute_external(ast, environment, io);
}

t_io_context evaluator_apply_io_file(t_ast *io_file, t_io_context io) {
  int fd;

  // Clean up previous redirections if needed
  if (io.needs_close_in && io.in_fd != STDIN_FILENO) {
    close(io.in_fd);
    io.in_fd = STDIN_FILENO;
    io.needs_close_in = false;
  }

  if (io.needs_close_out && io.out_fd != STDOUT_FILENO) {
    close(io.out_fd);
    io.out_fd = STDOUT_FILENO;
    io.needs_close_out = false;
  }

  switch (io_file->io_file.op->type) {
    case TOKEN_LESS:  // <
      fd = open(io_file->io_file.filename, O_RDONLY);
      if (fd == -1) {
        perror(io_file->io_file.filename);
        return io;
      }
      io.in_fd = fd;
      io.needs_close_in = true;
      break;

    case TOKEN_GREAT:  // >
      fd = open(io_file->io_file.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        perror(io_file->io_file.filename);
        return io;
      }
      io.out_fd = fd;
      io.needs_close_out = true;
      break;

    case TOKEN_DGREAT:  // >>
      fd = open(io_file->io_file.filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (fd == -1) {
        perror(io_file->io_file.filename);
        return io;
      }
      io.out_fd = fd;
      io.needs_close_out = true;
      break;

    case TOKEN_DLESS:  // << (Here document)
      // For simplicity in this version, not implementing heredoc
      fprintf(stderr, "Heredoc redirection not yet implemented\n");
      break;

    default:
      break;
  }

  return io;
}

void evaluator_close_io(t_io_context *io) {
  if (io->needs_close_in && io->in_fd != STDIN_FILENO) {
    close(io->in_fd);
    io->in_fd = STDIN_FILENO;
    io->needs_close_in = false;
  }

  if (io->needs_close_out && io->out_fd != STDOUT_FILENO) {
    close(io->out_fd);
    io->out_fd = STDOUT_FILENO;
    io->needs_close_out = false;
  }
}

bool evaluator_is_builtin(const char *cmd_name) {
  static const char *builtins[] = {"cd",     "echo", "env",   "exit",
                                   "export", "pwd",  "unset", NULL};

  for (int i = 0; builtins[i]; i++) {
    if (strcmp(cmd_name, builtins[i]) == 0) {
      return true;
    }
  }

  return false;
}

int evaluator_execute_builtin(t_ast *ast, t_hashmap *environment,
                              t_io_context io) {
  const char *cmd_name = ast->simple_command.cmd_name;
  int status = EXIT_SUCCESS;

  // Save stdin/stdout if needed for redirection
  int saved_stdin = -1;
  int saved_stdout = -1;

  if (io.in_fd != STDIN_FILENO) {
    saved_stdin = dup(STDIN_FILENO);
    dup2(io.in_fd, STDIN_FILENO);
  }

  if (io.out_fd != STDOUT_FILENO) {
    saved_stdout = dup(STDOUT_FILENO);
    dup2(io.out_fd, STDOUT_FILENO);
  }

  // Build argv
  char **argv = evaluator_build_argv(ast);
  if (!argv) {
    status = EXIT_FAILURE;
    goto cleanup;
  }

  // Execute the built-in command
  if (strcmp(cmd_name, "cd") == 0) {
    if (!argv[1]) {
      fprintf(stderr, "%s: cd: missing argument\n", MINISHELL_NAME);
      status = EXIT_FAILURE;
    } else if (chdir(argv[1]) == -1) {
      perror(argv[1]);
      status = EXIT_FAILURE;
    }
  } else if (strcmp(cmd_name, "exit") == 0) {
    // TODO: Implement proper exit with status code
    exit(EXIT_SUCCESS);
  } else if (strcmp(cmd_name, "env") == 0) {
    environment_print(environment);
  } else if (strcmp(cmd_name, "export") == 0) {
    // TODO: Implement export command
    fprintf(stderr, "export: not yet implemented\n");
  } else if (strcmp(cmd_name, "unset") == 0) {
    if (argv[1]) {
      environment_unset(environment, argv[1]);
    }
  } else if (strcmp(cmd_name, "echo") == 0) {
    bool newline = true;
    int i = 1;

    if (argv[i] && strcmp(argv[i], "-n") == 0) {
      newline = false;
      i++;
    }

    while (argv[i]) {
      if (i > 1 && (argv[i - 1] && strcmp(argv[i - 1], "-n") != 0)) {
        printf(" ");
      }
      printf("%s", argv[i]);
      i++;
    }

    if (newline) {
      printf("\n");
    }
  } else if (strcmp(cmd_name, "pwd") == 0) {
    char pwd[4096];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
      printf("%s\n", pwd);
    } else {
      perror("pwd");
      status = EXIT_FAILURE;
    }
  }

  // Free argv
  free(argv);

cleanup:
  // Restore stdin/stdout if redirected
  if (saved_stdin != -1) {
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);
  }

  if (saved_stdout != -1) {
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
  }

  // Close any redirected file descriptors
  evaluator_close_io(&io);

  return status;
}

int evaluator_execute_external(t_ast *ast, t_hashmap *environment,
                               t_io_context io) {
  pid_t pid = fork();

  if (pid == -1) {
    perror(MINISHELL_NAME);
    evaluator_close_io(&io);
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    // Child process

    // Set up redirections
    if (io.in_fd != STDIN_FILENO) {
      if (dup2(io.in_fd, STDIN_FILENO) == -1) {
        perror(MINISHELL_NAME);
        exit(EXIT_FAILURE);
      }
    }

    if (io.out_fd != STDOUT_FILENO) {
      if (dup2(io.out_fd, STDOUT_FILENO) == -1) {
        perror(MINISHELL_NAME);
        exit(EXIT_FAILURE);
      }
    }

    // Close file descriptors
    evaluator_close_io(&io);

    // Build argv and envp
    char **argv = evaluator_build_argv(ast);
    char **envp = evaluator_build_envp(environment);

    if (!argv || !envp) {
      fprintf(stderr, "%s: memory allocation error\n", MINISHELL_NAME);
      exit(EXIT_FAILURE);
    }

    // Execute the command
    execve(argv[0], argv, envp);

    // If execve returns, there was an error
    fprintf(stderr, "%s: command not found: %s\n", MINISHELL_NAME, argv[0]);

    // Clean up and exit
    free(argv);
    free(envp);
    exit(EXIT_FAILURE);
  }

  // Parent process
  evaluator_close_io(&io);

  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  return EXIT_FAILURE;
}

char **evaluator_build_argv(t_ast *ast) {
  // Count arguments
  int argc = 1;  // Start with 1 for the command name

  // Count words in cmd_suffix
  t_ast *suffix = ast->simple_command.cmd_suffix;
  while (suffix) {
    if (suffix->cmd_suffix.word) {
      argc++;
    }
    suffix = suffix->cmd_suffix.cmd_suffix;
  }

  // Allocate argv array (argc + 1 for NULL terminator)
  char **argv = ft_expect(calloc(argc + 1, sizeof(char *)), __func__);

  // Set command name
  argv[0] = (char *)ast->simple_command.cmd_name;

  // Add arguments from cmd_suffix
  int i = 1;
  suffix = ast->simple_command.cmd_suffix;
  while (suffix) {
    if (suffix->cmd_suffix.word) {
      argv[i++] = (char *)suffix->cmd_suffix.word;
    }
    suffix = suffix->cmd_suffix.cmd_suffix;
  }

  argv[argc] = NULL;
  return argv;
}

char **evaluator_build_envp(t_hashmap *environment) {
  // Count environment variables
  t_hashmap_iterator it = ft_hshbegin(environment);
  int env_count = 0;

  while (ft_hshnext(&it)) {
    env_count++;
  }

  // Allocate envp array (env_count + 1 for NULL terminator)
  char **envp = ft_expect(calloc(env_count + 1, sizeof(char *)), __func__);

  // Build envp entries
  it = ft_hshbegin(environment);
  int i = 0;

  while (ft_hshnext(&it)) {
    // Format: KEY=VALUE
    size_t key_len = strlen(it.key);
    size_t value_len = strlen((char *)it.value);
    char *entry = ft_expect(malloc(key_len + 1 + value_len + 1), __func__);

    strcpy(entry, it.key);
    entry[key_len] = '=';
    strcpy(entry + key_len + 1, (char *)it.value);

    envp[i++] = entry;
  }

  envp[env_count] = NULL;
  return envp;
}
