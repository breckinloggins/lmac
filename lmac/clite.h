//
//  clite.h
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_clite_h
#define lmac_clite_h

#include <stdio.h>

// Makes these easier to search for an minimize / eliminate later
#define global_variable static

#define ERR_NONE                0
#define ERR_USAGE               1
#define ERR_FILE_NOT_FOUND      2
#define ERR_LEX                 3

typedef enum {
#   define TOKEN(kind)  kind,
#   include "tokens.def.h"
} TokenKind;

typedef struct {
  const char *file;
  uint32_t line;
  uint8_t *range_start;
  uint8_t *range_end;
} SourceLocation;

typedef struct {
  TokenKind kind;
  SourceLocation location;
} Token;

typedef struct {
  const char *file;
  
  /* The entire contents of the current translation unit */
  uint8_t *buf;
  
  /* Current position indicators */
  uint8_t *pos;
  uint32_t line;
} Context;

const char *token_get_kind_name(TokenKind kind);
void token_fprint(FILE *f, Token t);

Token lexer_next_token(Context *ctx);

#endif
