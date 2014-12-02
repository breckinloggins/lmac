//
//  parser.c
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <stdlib.h>
#include <stdbool.h>

#include "clite.h"

#define IS_TOKEN_NONE(t) ((t).kind == TOK_NONE)

Context snapshot(Context *ctx) {
    Context saved = *ctx;
    
    return saved;
}

void restore(Context *ctx, Context snapshot) {
    *ctx = snapshot;
}

Token next_token(Context *ctx) {
    // TODO(bloggins): figure out how to incorporate comments and ws in generated
    // code
    for (;;) {
        Token t = lexer_next_token(ctx);
        if (t.kind == TOK_WS || t.kind == TOK_COMMENT) {
            continue;
        }
        
        if (t.kind == TOK_UNKOWN) {
            fprintf(stderr, "Unknown token\n");
            exit(ERR_LEX);
        }
        
        return t;
    }
}

Token accept_token(Context *ctx, TokenKind kind) {
    Token t = next_token(ctx);
    if (t.kind == kind) {
        return t;
    }
    
    lexer_put_back(ctx, t);
    
    return TOKEN_NONE;
}

Token expect_token(Context *ctx, TokenKind kind) {
    Token t = accept_token(ctx, kind);
    
    if (IS_TOKEN_NONE(t)) {
        fprintf(stderr, "error (line %d): expected %s\n", ctx->line, token_get_kind_name(kind));
        exit(ERR_PARSE);
    }
    
    return t;
}

bool parse_expression(Context *ctx) {
    Context s = snapshot(ctx);
    
    Token t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) {
        t = accept_token(ctx, TOK_NUMBER);
        if (IS_TOKEN_NONE(t)) {
            goto fail_parse;
        }
    }
    
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_defn_var(Context *ctx) {
    // TODO(bloggins): Snapshotting works but can be slow (because we might
    //                  backtrack a long way). Should we left-factor instead?
    Context s = snapshot(ctx);
    
    Token t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    t = accept_token(ctx, TOK_EQUALS);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    if (!parse_expression(ctx)) { goto fail_parse; }
    
    t = expect_token(ctx, TOK_SEMICOLON);
    
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_defn_fn(Context *ctx) {
    Context s = snapshot(ctx);

    // TODO(bloggins): Factor this grammar into reusable chunks like
    //                  "parse_begin_decl" and "parse_block"
    Token t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    t = accept_token(ctx, TOK_LPAREN);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    t = accept_token(ctx, TOK_RPAREN);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }

    
    // Parse the block
    t = expect_token(ctx, TOK_LBRACE);
    
    while (parse_defn_var(ctx)) {
        // Keep doing that
    }
    
    t = expect_token(ctx, TOK_RBRACE);
    
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

void parse_end(Context *ctx) {
    expect_token(ctx, TOK_END);
}

bool parse_toplevel(Context *ctx) {
    while (parse_defn_var(ctx) || parse_defn_fn(ctx)) {
        // Keep on doing that
    }
    
    parse_end(ctx);
    return true;
}

void parser_parse(Context *ctx) {
    if (!parse_toplevel(ctx)) {
        exit(ERR_PARSE);
    }
}
