//
//  lexer.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <ctype.h>

#include "clite.h"

// TODO(bloggins):
// This lexer should support unicode and UTF-8 encoding
// http://en.wikipedia.org/wiki/Punycode for UTF-8 identifiers?

SourceLocation lexed_source_location(Context *ctx) {
    SourceLocation sl = {0};
    sl.file = ctx->file;
    sl.line = ctx->line;
    sl.ctx = ctx;
    sl.range_start = ctx->pos;
    sl.range_end = ctx->pos + 1;
    
    return sl;
}

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
            SourceLocation sl = lexed_source_location(ctx);
            diag_printf(DIAG_ERROR, &sl, "invalid digit '%c' in number", ch);
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

void maybe_lex_keyword(Context *ctx, Token *t) {
    for (int i = TOK_KW_BEGIN + 1; i < TOK_KW_END; i++) {
        if (token_spelling_is_equivalent(*t, /*(TokenKind)*/i)) {
            t->kind = (TokenKind)i;
            return;
        }
    }
}

Token lexer_lex_chunk(Context *ctx, char end_of_chunk_marker, char chunk_marker_escape) {
    Token t = {};
    t.kind = TOK_NONE;
    t.location = lexed_source_location(ctx);
    
    bool in_escape = false;
    for (;;) {
        char ch = (char)*(ctx->pos++);
        if (!in_escape && (ch == chunk_marker_escape)) {
            // NOTE(bloggins): We read this in unprocessed because we always
            // store the EXACT spelling. Code that uses the spelling will need
            // to do the work to remove the escape character
            in_escape = true;
        } else if (in_escape) {
            in_escape = false;
        } else if (ch == end_of_chunk_marker) {
            break;
        }
    }
    
    t.location.range_end = ctx->pos;
    t.kind = TOK_CHUNK;
    return t;
}

Token lexer_peek_token(Context *ctx) {
    uint8_t *saved_pos = ctx->pos;
    Token t = lexer_next_token(ctx);
    
    ctx->pos = saved_pos;
    return t;
}

Token lexer_next_token_no_state(Context *ctx) {
    Token t = {};
    t.kind = TOK_UNKOWN;
    t.location.file = ctx->file;
    t.location.line = ctx->line;
    t.location.ctx = ctx;
    t.location.range_start = ctx->pos;
    t.location.range_end = ctx->pos;
    
    if (*(ctx->pos) == 0) {
        t.kind = TOK_END;
        goto finish;
    }
    
    // Lex single character lexemes
    char ch = (char)*ctx->pos;
    switch (ch) {
        case '#':
            t.kind = TOK_HASH;
            break;
        case '$':
            t.kind = TOK_DOLLAR;
            break;
        case ',':
            t.kind = TOK_COMMA;
            break;
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
        case '.':
            t.kind = TOK_DOT;
            if ((char)*(ctx->pos+1) == '.') {
                t.kind = TOK_DOT_DOT;
                ctx->pos++;
                if ((char)*(ctx->pos+1) == '.') {
                    t.kind = TOK_ELLIPSIS;
                    ctx->pos++;
                }
            }
            break;
        case '=':
            t.kind = TOK_EQUALS;
            if ((char)*(ctx->pos+1) == '=') {
                t.kind = TOK_EQUALS_EQUALS;
                ctx->pos++;
            }
            break;
        case '!':
            t.kind = TOK_BANG;
            if ((char)*(ctx->pos+1) == '=') {
                t.kind = TOK_BANG_EQUALS;
                ctx->pos++;
            }
            break;
        case '+':
            t.kind = TOK_PLUS;
            break;
        case '-':
            t.kind = TOK_MINUS;
            break;
        case '*':
            t.kind = TOK_STAR;
            break;
        case '/': {
            t.kind = TOK_FORWARDSLASH;
            
            // Comment?
            char ch2 = (char)*(ctx->pos+1);
            if (ch2 == '*') {
                t.kind = TOK_COMMENT;
                lex_multiline_comment(ctx);
            } else if (ch2 == '/') {
                t.kind = TOK_COMMENT;
                lex_singleline_comment(ctx);
            }

        } break;
        case '%':
            t.kind = TOK_PERCENT;
            break;
        case '<':
            t.kind = TOK_LANGLE;
            if ((char)*(ctx->pos+1) == '<') {
                t.kind = TOK_2LANGLE;
                ctx->pos++;
            } else if ((char)*(ctx->pos+1) == '=') {
                t.kind = TOK_LANGLE_EQUALS;
                ctx->pos++;
            }
            break;
        case '>':
            t.kind = TOK_RANGLE;
            if ((char)*(ctx->pos+1) == '>') {
                t.kind = TOK_2RANGLE;
                ctx->pos++;
            } else if ((char)*(ctx->pos+1) == '=') {
                t.kind = TOK_RANGLE_EQUALS;
                ctx->pos++;
            }
            break;
        case '&':
            t.kind = TOK_AMP;
            if ((char)*(ctx->pos+1) == '&') {
                t.kind = TOK_AMP_AMP;
                ctx->pos++;
            }
            break;
        case '|':
            t.kind = TOK_PIPE;
            if ((char)*(ctx->pos+1) == '|') {
                t.kind = TOK_PIPE_PIPE;
                ctx->pos++;
            }
            break;
        case '^':
            t.kind = TOK_CARET;
            break;
        case '\'':
            t.kind = TOK_QUOTE;
            break;
        case '"':
            t.kind = TOK_DOUBLE_QUOTE;
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
    
finish:
    t.location.range_end = ctx->pos;
    
    // NOTE(bloggins): We do this here because we need the token to have
    // a complete SourceLocation for proper spelling
    if (t.kind == TOK_IDENT) {
        maybe_lex_keyword(ctx, &t);
    }
    
    return t;
}

Token lexer_next_token(Context *ctx) {
    return lexer_next_token_no_state(ctx);
#if 0
    if (ctx->lex_mode == LEX_NORMAL) {
        return lexer_next_token_no_state(ctx);
    }
    
    for (;;) {
        Token t = lexer_next_token_no_state(ctx);
        if (t.kind == TOK_HASH || t.kind == TOK_END) {
            return t;
        }
    }
#endif
}

void lexer_put_back(Context *ctx, Token token) {
    ctx->line = token.location.line;
    ctx->pos = token.location.range_start;
}
