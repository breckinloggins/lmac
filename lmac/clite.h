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
#include <stdlib.h>

// Makes these easier to search for an minimize / eliminate later
#define global_variable static

#define ERR_NONE                0
#define ERR_USAGE               1
#define ERR_FILE_NOT_FOUND      2
#define ERR_LEX                 3
#define ERR_PARSE               4
#define ERR_VISIT               5

typedef enum {
#   define TOKEN(kind)  kind,
#   include "tokens.def.h"
} TokenKind;

typedef enum {
#   define AST(kind, ...) AST_##kind,
#   include "ast.def.h"
} ASTKind;

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

extern const Token TOKEN_NONE;

struct ASTBase;
struct ASTList;
typedef struct ASTList {
    struct ASTBase *node;
    struct ASTList *next;
} ASTList;

#define VISIT_OK        0
typedef int (*VisitFn)(struct ASTBase *node, void *ctx);

typedef struct ASTBase {
    ASTKind kind;
    int (*accept)(struct ASTBase *node, VisitFn visitor, void *ctx);
} ASTBase;

typedef struct {
    ASTBase base;
} ASTDefn;

typedef struct {
    ASTBase base;
    
    ASTList *definitions;
} ASTTopLevel;

typedef struct {
    const char *file;
    
    /* The entire contents of the current translation unit */
    uint8_t *buf;
    
    /* Current position indicators */
    uint8_t *pos;
    uint32_t line;
    
    /* Parsed AST */
    ASTTopLevel *ast;
} Context;

const char *token_get_kind_name(TokenKind kind);
void token_fprint(FILE *f, Token t);

Token lexer_next_token(Context *ctx);
Token lexer_peek_token(Context *ctx);
void lexer_put_back(Context *ctx, Token token);

void parser_parse(Context *ctx);

// AST Creation functions
#define AST(_, name, type)                          \
type *ast_create_##name();
#include "ast.def.h"

const char *ast_get_kind_name(ASTKind kind);
int ast_visit(ASTBase *node, VisitFn visitor, void *ctx);
void ast_list_add(ASTList **list, ASTBase *node);

#endif
