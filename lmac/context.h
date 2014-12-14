//
//  context.h
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_context_h
#define lmac_context_h

#include "ast.h"
#include "scope.h"

/*
 * Compiler Context
 */
typedef struct Context {
    /* must be first in struct */
    int magic;
    
    const char *file;
    List *system_header_paths;
    
    /* The entire contents of the current translation unit */
    uint8_t *buf;
    size_t buf_size;
    
    /* Current position indicators */
    uint8_t *pos;
    uint32_t line;
    
    struct {
        bool lex_keywords_as_identifiers;
    } lex_mode;
    
    /* TODO(bloggins): Turn this into a list and control error termination
     * better */
    int last_error;
    
    Scope *active_scope;
    List *pp_defines;
    
    /* Parsed AST */
    ASTBase *ast;
} Context;

Context *context_create();
Scope *context_scope_push(Context *ctx);
Scope *context_scope_pop(Context *ctx);
void context_load_file(Context *ctx, const char *filename);

#endif
