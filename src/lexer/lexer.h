#ifndef LEXER_H
#define LEXER_H

#include "token/token.h"

typedef struct s_lexer t_lexer;

t_lexer *lexer_new(const char *input);
void lexer_free(t_lexer *lexer);
t_token *lexer_next_token(t_lexer *lexer);

#endif
