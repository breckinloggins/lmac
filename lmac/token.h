//
//  token.h
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_token_h
#define lmac_token_h

#include "token.h"

typedef enum {
#   define DIAG_KIND(kind, ...)  DIAG_##kind,
#   include "diag.def.h"
} DiagKind;

typedef enum {
#   define TOKEN(kind, ...)  kind,
#   include "tokens.def.h"
} TokenKind;

typedef struct {
    uint8_t *start;
    uint8_t *end;
} Spelling;

typedef struct {
    const char *file;
    uint32_t line;
    
    union {
        struct {
            uint8_t *range_start;
            uint8_t *range_end;
        };
        Spelling spelling;
    };
} SourceLocation;

typedef struct {
    TokenKind kind;
    SourceLocation location;
} Token;

extern const Token TOKEN_NONE;

size_t spelling_strlen(Spelling spelling);
bool spelling_streq(Spelling spelling, const char *str);
bool spelling_equal(Spelling spelling1, Spelling spelling2);
void spelling_fprint(FILE *f, Spelling spelling);

/* spelling_cstring returns a shared pointer to a string that will be
 * overwritten the next time spelling_cstring is called. Be sure to duplicate
 * the string if you need to keep it.
 */
const char *spelling_cstring(Spelling spelling);

const char *token_get_name(TokenKind kind);
size_t token_strlen(Token t);
bool token_streq(Token t, const char *str);
void token_fprint(FILE *f, Token t);


#endif
