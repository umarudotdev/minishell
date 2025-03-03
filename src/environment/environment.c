#include "environment.h"

#include <stdio.h>

#include "ft_ansi.h"
#include "ft_hashmap.h"
#include "ft_stdlib.h"
#include "ft_string.h"

t_hashmap *environment_new(const char **variables) {
  t_hashmap *environment = ft_expect(ft_hshnew(NULL), __func__);
  for (size_t i = 0; variables[i] != NULL; ++i) {
    environment_set(environment, variables[i]);
  }

  return environment;
}

void environment_free(t_hashmap *environment) {
  t_hashmap_iterator it = ft_hshbegin(environment);
  while (ft_hshnext(&it)) {
    environment_unset(environment, it.key);
  }
  ft_hshfree(environment);
}

void environment_set(t_hashmap *environment, const char *str) {
  const char *equal_sign = ft_strchr(str, '=');
  t_string name;
  t_string value = NULL;
  if (equal_sign) {
    name = ft_expect(ft_stnnew_size(str, equal_sign - str), __func__);
    value = ft_expect(ft_stnnew(equal_sign + 1), __func__);
  } else {
    name = ft_expect(ft_stnnew(str), __func__);
  }

  environment_unset(environment, name);
  if (!ft_hshset(environment, name, value)) {
    ft_panic(__func__);
  }
}

void environment_unset(t_hashmap *environment, const char *name) {
  ft_hshdel2(environment, name, ft_stnfree, ft_stnfree);
}

void environment_print(const t_hashmap *environment) {
  static const int indent_size = 2;

  t_hashmap_iterator it = ft_hshbegin(environment);
  printf("<" ANSI_MAGENTA "environment" ANSI_RESET ">\n");
  while (ft_hshnext(&it)) {
    printf("%*s<" ANSI_MAGENTA "variable" ANSI_RESET " " ANSI_CYAN
           "name" ANSI_RESET "=" ANSI_YELLOW "\"%s\"" ANSI_RESET " " ANSI_CYAN
           "value" ANSI_RESET "=" ANSI_YELLOW "\"%s\"" ANSI_RESET " />\n",
           indent_size, "", it.key, (char *)it.value);
  }
  printf("</" ANSI_MAGENTA "environment" ANSI_RESET ">\n");
}
