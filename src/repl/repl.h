#ifndef REPL_H
#define REPL_H

#include "environment/environment.h"

/**
 * @brief Starts the REPL (Read-Eval-Print-Loop).
 *
 * @param environment The environment variables.
 */
void repl_start(t_hashmap *environment);

#endif
