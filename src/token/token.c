#include "token.h"

#include <stdio.h>
#include <stdlib.h>

#include "ft_ansi.h"
#include "ft_stdlib.h"

t_token *token_new(t_token token) {
  t_token *new_token = ft_expect(malloc(sizeof(t_token)), __func__);
  *new_token = token;
  return new_token;
}

void token_free(t_token *token) { free(token); }

const char *token_type_to_string(t_token_type type) {
  return (const char *[]){
      [TOKEN_ILLEGAL] = "illegal", [TOKEN_WORD] = "word",
      [TOKEN_NEWLINE] = "newline", [TOKEN_SEMI] = "semi",
      [TOKEN_AND_IF] = "and-if",   [TOKEN_OR_IF] = "or-if",
      [TOKEN_PIPE] = "pipe",       [TOKEN_LPAREN] = "lparen",
      [TOKEN_RPAREN] = "rparen",   [TOKEN_LESS] = "less",
      [TOKEN_GREAT] = "great",     [TOKEN_DLESS] = "dless",
      [TOKEN_DGREAT] = "dgreat",
  }[type];
}

void token_print(t_token *token) {
  printf("<" ANSI_MAGENTA "token" ANSI_RESET " " ANSI_CYAN "type" ANSI_RESET
         "=" ANSI_YELLOW "\"%s\"" ANSI_RESET " " ANSI_CYAN "literal" ANSI_RESET
         "=" ANSI_YELLOW "\"%s\"" ANSI_RESET " />\n",
         token_type_to_string(token->type), token->literal);
}
