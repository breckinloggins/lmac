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
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Makes these easier to search for an minimize / eliminate later
#define global_variable static

#define ERR_NONE                0
#define ERR_USAGE               1
#define ERR_FILE_NOT_FOUND      2
#define ERR_LEX                 3
#define ERR_PARSE               4
#define ERR_VISIT               5
#define ERR_ANALYZE             6
#define ERR_CODEGEN             7
#define ERR_CC                  8

typedef enum {
#   define DIAG_KIND(kind, ...)  DIAG_##kind,
#   include "diag.def.h"
} DiagKind;

typedef enum {
#   define TOKEN(kind, ...)  kind,
#   include "tokens.def.h"
} TokenKind;

typedef enum {
#   define AST(kind, ...) AST_##kind,
#   include "ast.def.h"
} ASTKind;

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

struct ASTBase;
struct ASTList;
typedef struct ASTList {
    struct ASTBase *node;
    struct ASTList *next;
} ASTList;

#define ASTLIST_FOREACH(type, var_name, ast_list, block)                    \
do {                                                                        \
    ASTList *list_copy = ast_list;                                          \
    while((list_copy) != NULL) {                                            \
        type var_name = (type)(list_copy)->node;                            \
        assert(var_name != NULL);                                           \
        list_copy = list_copy->next;                                        \
        block                                                               \
    }                                                                       \
} while(0);

typedef enum {
    VISIT_PRE,
    VISIT_POST
} VisitPhase;

#define VISIT_OK        0
typedef int (*VisitFn)(struct ASTBase *node, VisitPhase phase, void *ctx);

typedef struct ASTBase {
    ASTKind kind;
    SourceLocation location;
    struct ASTBase *parent;
    
    int (*accept)(struct ASTBase *node, VisitFn visitor, void *ctx);
} ASTBase;

typedef struct {
    ASTBase base;
    
    ASTList *statements;
} ASTBlock;

typedef struct {
    ASTBase base;
} ASTIdent;

typedef struct {
    ASTBase base;
    char op;
} ASTOperator;

typedef struct {
    ASTBase base;
    
    ASTIdent *type;
    ASTIdent *name;
    ASTBlock *block;
    
} ASTDefnFunc;

typedef struct {
    ASTBase base;
    
    ASTIdent *name;
} ASTExprIdent;

typedef struct {
    ASTBase base;
    
    int number;
} ASTExprNumber;

typedef struct {
    ASTBase base;
    
    ASTBase *left;
    ASTOperator *op;
    ASTBase *right;
} ASTExprBinary;

typedef struct {
    ASTBase base;
    
    ASTIdent *type;
    ASTIdent *name;
    
    // TODO(bloggins): Need ASTExpression union
    ASTBase *expression;
    
} ASTDefnVar;

typedef struct {
    ASTBase base;
    
    // TODO(bloggins): Need ASTExpression union
    ASTBase *expression;
} ASTStmtReturn;

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

const char *diag_get_name(DiagKind kind);
void diag_printf(DiagKind kind, SourceLocation* loc, const char *fmt, ...);

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
void ast_fprint(FILE *f, ASTBase *node, int indent);

void analyzer_analyze(ASTTopLevel *ast);

void codegen_generate(FILE *f, ASTTopLevel *ast);

#endif
