//
//  clite.h
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// Umbrella header file for the compiler. Most code should just include this.

#ifndef lmac_clite_h
#define lmac_clite_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "token.h"
#include "ast.h"
#include "act.h"
#include "scope.h"
#include "context.h"

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

/*
 * Public Interface
 */

const char *diag_get_name(DiagKind kind);
void diag_printf(DiagKind kind, SourceLocation* loc, const char *fmt, ...);
extern int diag_errno;

Token lexer_next_token(Context *ctx);
Token lexer_peek_token(Context *ctx);
void lexer_put_back(Context *ctx, Token token);

void parser_parse(Context *ctx);

// AST Creation functions
#define AST(_, name, type)                          \
type *ast_create_##name();
#include "ast.def.h"

void analyzer_analyze(ASTTopLevel *ast);

void codegen_generate(FILE *f, ASTTopLevel *ast);

#endif
