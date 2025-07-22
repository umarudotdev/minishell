#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast/ast.h"
#include "environment/environment.h"

/**
 * @brief Evaluates an AST and executes the commands.
 *
 * @param ast The abstract syntax tree to evaluate.
 * @param environment The environment variables.
 * @return The exit status of the last command executed.
 */
int evaluator_evaluate(t_ast *ast, t_hashmap *environment);

#endif
