//
//  token.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

global_variable const char *g_token_names[] = {
#   define TOKEN(_, name, ...) #name,
#   include "tokens.def.h"
};

const char *token_get_name(TokenKind kind) {
    return g_token_names[kind];
}

const Token TOKEN_NONE = {TOK_NONE};

size_t token_strlen(Token t) {
    return spelling_strlen(t.location.spelling);
}

bool token_streq(Token t, const char *str) {
    return spelling_streq(t.location.spelling, str);
}

bool token_spelling_is_equivalent(Token t, TokenKind kind) {
    return spelling_streq(t.location.spelling, g_token_names[kind]);
}

void token_fprint(FILE *f, Token t) {
    fprintf(f, "{\n");
    fprintf(f, "\ttype: \"Token\",\n");
    fprintf(f, "\tname: \"%s\",\n", token_get_name(t.kind));
    fprintf(f, "\tline: %d,\n", t.location.line);
    if (t.kind == TOK_COMMENT || t.kind == TOK_IDENT || t.kind == TOK_NUMBER ||
        t.kind == TOK_UNKOWN) {
        fprintf(f, "\tcontent: \"");
        spelling_fprint(f, t.location.spelling);
        fprintf(f, "\",\n");
    }
    fprintf(f, "}\n");
}

bool token_is_of_kind(Token t, TokenKind *kinds) {
    size_t idx = 0;
    for (;;) {
        if (kinds[idx] == TOK_LAST) {
            return false;
        }
        
        if (t.kind == kinds[idx]) {
            return true;
        }
        
        ++idx;
    }
    
    return false;
}
