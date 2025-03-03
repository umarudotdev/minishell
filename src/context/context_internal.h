#ifndef CONTEXT_INTERNAL_H
#define CONTEXT_INTERNAL_H

#include "ft_hashmap.h"
#include "ft_string.h"

struct s_context {
  t_hashmap *environment;
  t_hashmap *path;
};

#endif
