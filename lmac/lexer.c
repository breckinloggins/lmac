//
//  lexer.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

Token lexer_next_token(Context *ctx) {
  Token t;
  t.kind = TOK_UNKOWN;
  t.location.file = ctx->file;
  t.location.line = ctx->line;
  t.location.range_start = ctx->pos;
  t.location.range_end = ctx->pos;
  
  if (*(ctx->pos) == 0) {
    t.kind = TOK_END;
    goto finish;
  }
  
  // Lex single character lexemes
  char ch = (char)*ctx->pos;
  switch (ch) {
    case ';':
      t.kind = TOK_SEMICOLON;
      break;
    case '{':
      t.kind = TOK_LBRACE;
      break;
    case '}':
      t.kind = TOK_RBRACE;
      break;
    case '(':
      t.kind = TOK_LPAREN;
      break;
    case ')':
      t.kind = TOK_RPAREN;
      break;
    case '=':
      t.kind = TOK_EQUALS;
      break;
    case ' ': case '\t':
      t.kind = TOK_WS;
      break;
    case '\n':
      t.kind = TOK_WS;
      ctx->line++;
      break;
      
    default:
      break;
  }
  
  ctx->pos++;
  
  if (t.kind != TOK_UNKOWN) {
    // We got a valid token
    goto finish;
  }
  
  if (*(ctx->pos) == 0) {
    t.kind = TOK_END;
    goto finish;
  }
  
  // Lex double character lexemes
  char ch2 = (char)*ctx->pos;
  if (ch == '/' && ch2 == '*') {
    t.kind = TOK_COMMENT;
    for (;;) {
      // Gobble the comment to next '*/'
      char c = (char)*(++ctx->pos);
      if (c == '\n') {
        // TODO(bloggins): We're doing this in too many places. Abstract to a macro
        ctx->line++;
      } else if (c == '*') {
        if ((char)*(++ctx->pos) == '/') {
          break;
        }
      }
    }
  } else if (ch == '/' && ch2 == '/') {
    t.kind = TOK_COMMENT;
    for (;;) {
      // Gobble the comment to EOL or EOI
      char c = (char)*(++ctx->pos);
      if (c == '\n' || c == 0) {
        ctx->line++;
        break;
      }
    }
  } else {
    t.kind = TOK_IDENT;
  }
  
  ctx->pos++;
  
finish:
  t.location.range_end = ctx->pos;
  return t;
}
