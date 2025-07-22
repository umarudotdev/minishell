#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "ft_hashmap.h"

t_hashmap *environment_new(const char **variables);
void environment_free(t_hashmap *environment);
void environment_set(t_hashmap *environment, const char *str);
void environment_unset(t_hashmap *environment, const char *name);
void environment_print(const t_hashmap *environment);

#endif
