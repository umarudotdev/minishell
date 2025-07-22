#ifndef LEXER_INTERNAL_H
#define LEXER_INTERNAL_H

#include <stddef.h>

#include "ft_arraylist.h"
#include "lexer.h"
#include "token/token.h"

struct s_lexer {
  const char *input;
  size_t input_length;
  size_t position;
  size_t read_position;
  char c;
  t_array *tokens;
};

t_token *lexer_read_word(t_lexer *lexer);
void lexer_advance(t_lexer *lexer);
char lexer_peek(const t_lexer *lexer);
void lexer_skip_whitespace(t_lexer *lexer);
bool lexer_is_at_end(const t_lexer *lexer);
bool lexer_is_metacharacter(char c);
bool lexer_is_quoting(char c);

#endif
