#include <stdlib.h>

#include "environment/environment.h"
#include "repl/repl.h"

int main(void) {
  extern const char **environ;

  t_hashmap *environment = environment_new(environ);
  repl_start(environment);
  environment_free(environment);

  return EXIT_SUCCESS;
}
