#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stddef.h>

typedef enum e_token_type {
  TOKEN_ILLEGAL,
  TOKEN_EOF,
  TOKEN_WORD,
  TOKEN_NEWLINE,
  TOKEN_SEMI,
  TOKEN_AND_IF,
  TOKEN_OR_IF,
  TOKEN_PIPE,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LESS,
  TOKEN_GREAT,
  TOKEN_DLESS,
  TOKEN_DGREAT,
} t_token_type;

typedef struct s_token {
  t_token_type type;
  const char* literal;
} t_token;

t_token* token_new(t_token token);
void token_free(t_token* token);
const char* token_type_to_string(t_token_type type);
void token_print(t_token* token);

#endif
