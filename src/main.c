#include <stdio.h>
#include <stdlib.h>

#include "environment/environment.h"
#include "repl/repl.h"

int main(void) {
  extern const char **environ;

  // Initialize environment
  t_hashmap *environment = environment_new(environ);

  // Start the REPL
  repl_start(environment);

  // Clean up
  environment_free(environment);

  return EXIT_SUCCESS;
}
