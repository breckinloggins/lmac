//
//  lexer.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <ctype.h>
#include <stdlib.h>

#include "clite.h"

void lex_singleline_comment(Context *ctx) {
    for (;;) {
        // Gobble the comment to EOL or EOI
        char c = (char)*(++ctx->pos);
        if (c == '\n' || c == 0) {
            ctx->line++;
            break;
        }
    }
}

void lex_multiline_comment(Context *ctx) {
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
}

void lex_number(Context *ctx) {
    for (;;) {
        char ch = (char)*(++ctx->pos);
        if (isdigit(ch)) {
            continue;
        }
        
        if (isalpha(ch)) {
            // TODO(bloggins): Extract error reporting system
            fprintf(stderr, "error (line %d): invalid digit '%c' in number\n", ctx->line, ch);
            exit(ERR_LEX);
        }
        
        --ctx->pos;
        break;
    }
    
}

void lex_ident(Context *ctx) {
    for (;;) {
        char ch = (char)*(++ctx->pos);
        if (isalnum(ch) || ch == '_') {
            continue;
        }
        
        --ctx->pos;
        break;
    }
}

Token lexer_peek_token(Context *ctx) {
    uint8_t *saved_pos = ctx->pos;
    Token t = lexer_next_token(ctx);
    
    ctx->pos = saved_pos;
    return t;
}

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
        case '+':
            t.kind = TOK_PLUS;
            break;
        case ' ': case '\t':
            t.kind = TOK_WS;
            break;
        case '\n':
            t.kind = TOK_WS;
            ctx->line++;
            break;
            
        default:
            if (isdigit(ch)) {
                t.kind = TOK_NUMBER;
                lex_number(ctx);
            } else if(isalpha(ch) || ch == '_') {
                t.kind = TOK_IDENT;
                lex_ident(ctx);
            }
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
        lex_multiline_comment(ctx);
    } else if (ch == '/' && ch2 == '/') {
        t.kind = TOK_COMMENT;
        lex_singleline_comment(ctx);
    } else {
        t.kind = TOK_UNKOWN;
    }
    
    ctx->pos++;
    
finish:
    t.location.range_end = ctx->pos;
    return t;
}

void lexer_put_back(Context *ctx, Token token) {
    ctx->line = token.location.line;
    ctx->pos = token.location.range_start;
}
