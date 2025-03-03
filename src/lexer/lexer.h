#ifndef LEXER_H
#define LEXER_H

#include "token/token.h"

#define new$lexer(input) lexer_new(input)
#define free$lexer(lexer) lexer_free(lexer)

typedef struct s_lexer t_lexer;

t_lexer *lexer_new(const char *input);
void lexer_free(t_lexer *lexer);
t_token *lexer_next_token(t_lexer *lexer);

#endif
