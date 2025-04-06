#define _POSIX_C_SOURCE 200809L

#include "repl_internal.h"

#include <stdio.h>
#include <readline/readline.h>
#include <signal.h>

// Global flag for signal handling
volatile sig_atomic_t g_sigint_received = 0;

/**
 * @brief Signal handler for SIGINT (Ctrl+C).
 *
 * @param sig The signal number.
 */
static void sigint_handler(int sig) {
  (void)sig;
  g_sigint_received = 1;
  printf("\n");
  rl_on_new_line();
  rl_replace_line("", 0);
  rl_redisplay();
}

void repl_setup_signals(void) {
  // Set up SIGINT handler (Ctrl+C)
  struct sigaction sa_int;
  sa_int.sa_handler = sigint_handler;
  sa_int.sa_flags = 0;
  sigemptyset(&sa_int.sa_mask);
  sigaction(SIGINT, &sa_int, NULL);

  // Ignore SIGQUIT (Ctrl+\)
  signal(SIGQUIT, SIG_IGN);
}
