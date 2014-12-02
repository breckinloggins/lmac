//
//  main.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ERR_NONE                0
#define ERR_USAGE               1
#define ERR_FILE_NOT_FOUND      2
#define ERR_LEX                 3

// Makes these easier to search for an minimize / eliminate later
#define global_variable static

typedef enum {
#   define TOKEN(kind)  kind,
#   include "tokens.def.h"
} TokenKind;

global_variable const char *g_token_names[] = {
#   define TOKEN(kind) #kind,
#   include "tokens.def.h"
};

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

// INFO(bloggins): I normally don't use any kind of "hungarian" prefixes
// on variables, but I make an exception for globals. They should be rare
// and they should stand out.
global_variable Context g_ctx;

void fprint_token(FILE *f, Token t) {
    fprintf(f, "{\n");
    fprintf(f, "\ttype: \"Token\",\n");
    fprintf(f, "\tkind: \"%s\",\n", g_token_names[t.kind]);
    fprintf(f, "\tline: %d,\n", t.location.line);
    if (t.kind == TOK_COMMENT || t.kind == TOK_IDENT) {
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

Token next_token(Context *ctx) {
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

int main(int argc, const char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: lmac <file>\n");
        return ERR_USAGE;
    }
    
    // Make sure the file exists
    const char *file = argv[1];
    if (access(file, R_OK) == -1) {
        fprintf(stderr, "error: input file not found (%s)\n", file);
        return ERR_FILE_NOT_FOUND;
    }
    
    // TODO(bloggins): this is probably not the most memory efficient thing we could do
    FILE *fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    g_ctx.buf = (uint8_t *)malloc(fsize + 1);
    fread(g_ctx.buf, fsize, 1, fp);
    fclose(fp);
    
    // Ensure null-termination
    g_ctx.buf[fsize] = 0;
    g_ctx.pos = g_ctx.buf;
    g_ctx.line = 1;
    
    for (;;) {
        Token t = next_token(&g_ctx);
        if (t.kind == TOK_WS) {
            continue;
        }
        
        fprint_token(stderr, t);
        
        if (t.kind == TOK_END) {
            break;
        }
    }
    
    // NOTE(bloggins): Sanity check
    if (g_ctx.pos - g_ctx.buf < fsize) {
        fprintf(stderr, "Unexpected end of input\n");
        return ERR_LEX;
    }
    
    // NOTE(bloggins): We aren't freeing anything in the global context. There's no point
    // since the OS does that for us anyway and we don't want to take any longer to exit
    // than we need to.
    return ERR_NONE;
}
