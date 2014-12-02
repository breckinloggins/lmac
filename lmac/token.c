//
//  token.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

global_variable const char *g_token_names[] = {
#   define TOKEN(kind) #kind,
#   include "tokens.def.h"
};

const char *token_get_kind_name(TokenKind kind) {
    return g_token_names[kind];
}

const Token TOKEN_NONE = {TOK_NONE};

void token_fprint(FILE *f, Token t) {
    fprintf(f, "{\n");
    fprintf(f, "\ttype: \"Token\",\n");
    fprintf(f, "\tkind: \"%s\",\n", token_get_kind_name(t.kind));
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
