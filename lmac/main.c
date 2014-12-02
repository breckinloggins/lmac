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
    TOK_UNKOWN = 0,
    
    /* Everything that isn't lexed as a specific token is
     * characterized as an "identifier". This includes types, 
     * variables, and functions but also things like numeric
     * literals. This keeps the lexer or more sane and flexible
     * and moves things like "can't assign a type to a variable"
     * into the semantic analyzer
     */
    TOK_IDENT,
    
    /* Tokens for "structural symbols" like '{' and ';' */
    TOK_SEMICOLON,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    
    /* Our tokenizer is whitespace and comment preserving 
     * to better map input code to generated code
     */
    TOK_WS,
    TOK_COMMENT,
    
    /* Annotation token for end of input */
    TOK_END,
    
    /* Should ALWAYS be last */
    TOK_LAST,
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
    
    /* The position of the lex / parse cursor */
    uint8_t *pos;
} Context;

// INFO(bloggins): I normally don't use any kind of "hungarian" prefixes
// on variables, but I make an exception for globals. They should be rare
// and they should stand out.
global_variable Context g_ctx;

Token next_token(Context *ctx) {
    Token t;
    t.kind = TOK_UNKOWN;
    t.location.file = ctx->file;
    t.location.range_start = ctx->pos;
    t.location.range_end = ctx->pos;
    
    if (*(ctx->pos) == 0) {
        t.kind = TOK_END;
    } else {
        ctx->pos++;
    }
    
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
    
    for (;;) {
        Token t = next_token(&g_ctx);
        fprintf(stderr, "TOKEN: %d\n", t.kind);
        
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
