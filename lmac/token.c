//
//  token.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#include <string.h>

global_variable const char *g_token_names[] = {
#   define TOKEN(_, name, ...) #name,
#   include "tokens.def.h"
};

const char *token_get_name(TokenKind kind) {
    return g_token_names[kind];
}

const Token TOKEN_NONE = {TOK_NONE};

size_t token_strlen(Token t) {
    return (t.location.range_end - t.location.range_start);
}

bool token_streq(Token t, const char *str) {
    size_t tok_len = token_strlen(t);
    if (tok_len != strlen(str)) {
        return false;
    }
    
    const char *c = str;
    while (*c) {
        size_t off = c - str;
        if (t.location.range_start + off > t.location.range_end) {
            // Ran off the end of the token, can't be equal
            return false;
        }
        
        char tc = (char)*(t.location.range_start + off);
        if (*c != tc) {
            return false;
        }
        
        ++c;
    }
    
    return true;
}

void token_fprint(FILE *f, Token t) {
    fprintf(f, "{\n");
    fprintf(f, "\ttype: \"Token\",\n");
    fprintf(f, "\tname: \"%s\",\n", token_get_name(t.kind));
    fprintf(f, "\tline: %d,\n", t.location.line);
    if (t.kind == TOK_COMMENT || t.kind == TOK_IDENT || t.kind == TOK_NUMBER ||
        t.kind == TOK_UNKOWN) {
        fprintf(f, "\tcontent: \"");
        for (char *cp = (char*)t.location.range_start; cp < (char*)t.location.range_end; cp++) {
            if (*cp == '\n') {
                fprintf(f, "\\n");
            } else if (*cp == '\t') {
                fprintf(f, "\\t");
            } else {
                fprintf(f, "%c", *cp);
            }
        }
        fprintf(f, "\",\n");
    }
    fprintf(f, "}\n");
}
