#include "lexer.h"

#include <stdbool.h>
#include <stdlib.h>

#include "ft_arraylist.h"
#include "ft_ctype.h"
#include "ft_stdlib.h"
#include "ft_string.h"
#include "lexer_internal.h"
#include "token/token.h"

t_lexer *lexer_new(const char *input) {
  t_lexer *lexer = ft_expect(malloc(sizeof(t_lexer)), __func__);
  *lexer = (t_lexer){
      .input = input,
      .input_length = ft_strlen(input),
      .position = 0,
      .read_position = 0,
      .c = '\0',
      .tokens = ft_expect(ft_arrnew(sizeof(t_token **)), __func__),
  };
  lexer_advance(lexer);
  return lexer;
}

void lexer_free(t_lexer *lexer) {
  for (size_t i = 0; i < ft_arrsize(lexer->tokens); ++i) {
    t_token **token = ft_arrat(lexer->tokens, i);
    ft_stnfree((t_string)(*token)->literal);
    token_free(*token);
  }
  ft_arrfree(lexer->tokens);
  free(lexer);
}

#include <stdio.h>

t_token *lexer_next_token(t_lexer *lexer) {
  t_token token;

  lexer_skip_whitespace(lexer);

  switch (lexer->c) {
    case ';':
      token = (t_token){.type = TOKEN_SEMI, .literal = ";"};
      break;
    case '&':
      if (lexer_peek(lexer) == '&') {
        lexer_advance(lexer);
        token = (t_token){.type = TOKEN_AND_IF, .literal = "&&"};
      } else {
        token = (t_token){.type = TOKEN_ILLEGAL, .literal = "&"};
      }
      break;
    case '|':
      if (lexer_peek(lexer) == '|') {
        lexer_advance(lexer);
        token = (t_token){.type = TOKEN_OR_IF, .literal = "||"};
      } else {
        token = (t_token){.type = TOKEN_PIPE, .literal = "|"};
      }
      break;
    case '(':
      token = (t_token){.type = TOKEN_LPAREN, .literal = "("};
      break;
    case ')':
      token = (t_token){.type = TOKEN_RPAREN, .literal = ")"};
      break;
    case '<':
      if (lexer_peek(lexer) == '<') {
        lexer_advance(lexer);
        token = (t_token){.type = TOKEN_DLESS, .literal = "<<"};
      } else {
        token = (t_token){.type = TOKEN_LESS, .literal = "<"};
      }
      break;
    case '>':
      if (lexer_peek(lexer) == '>') {
        lexer_advance(lexer);
        token = (t_token){.type = TOKEN_DGREAT, .literal = ">>"};
      } else {
        token = (t_token){.type = TOKEN_GREAT, .literal = ">"};
      }
      break;
    case '\0':
      token = (t_token){.type = TOKEN_NEWLINE, .literal = "<newline>"};
      break;
    default: {
      t_token *new_token = lexer_read_word(lexer);

      if (!ft_arrappend(lexer->tokens, &new_token)) {
        ft_panic(__func__);
      }

      return new_token;
    }
  }

  lexer_advance(lexer);

  t_token *new_token = token_new((t_token){
      .type = token.type,
      .literal = ft_expect(ft_stnnew(token.literal), __func__),
  });

  if (!ft_arrappend(lexer->tokens, &new_token)) {
    ft_panic(__func__);
  }

  return new_token;
}

t_token *lexer_read_word(t_lexer *lexer) {
  size_t start = lexer->position;
  char quote = '\0';

  while (!lexer_is_at_end(lexer) &&
         (!lexer_is_metacharacter(lexer->c) || quote != '\0')) {
    if (lexer->c == '\\') {
      lexer_advance(lexer);
      if (!lexer_is_at_end(lexer)) lexer_advance(lexer);
      continue;
    }
    if (lexer_is_quoting(lexer->c)) {
      if (quote == '\0') {
        quote = lexer->c;
        lexer_advance(lexer);
        continue;
      } else if (quote == lexer->c) {
        quote = '\0';
        lexer_advance(lexer);
        continue;
      }
    }
    lexer_advance(lexer);
  }

  size_t end = lexer->position;

  return token_new((t_token){
      .type = TOKEN_WORD,
      .literal = ft_expect(ft_stnnew_size(lexer->input + start, end - start),
                           __func__),
  });
}

void lexer_advance(t_lexer *lexer) {
  lexer->c = lexer_peek(lexer);
  lexer->position = lexer->read_position;
  ++lexer->read_position;
}

char lexer_peek(const t_lexer *lexer) {
  if (lexer->read_position >= lexer->input_length) {
    return '\0';
  }
  return lexer->input[lexer->read_position];
}

void lexer_skip_whitespace(t_lexer *lexer) {
  while (ft_isspace(lexer->c)) {
    lexer_advance(lexer);
  }
}

bool lexer_is_at_end(const t_lexer *lexer) { return lexer->c == '\0'; }

bool lexer_is_metacharacter(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '|' || c == '&' ||
         c == ';' || c == '(' || c == ')' || c == '<' || c == '>';
}

bool lexer_is_quoting(char c) { return c == '\'' || c == '"'; }
