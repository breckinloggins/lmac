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

typedef enum ContextKind {
    CONTEXT_KIND_NONE = 0,
    CONTEXT_KIND_COMPILE,
    //CONTEXT_ACTION_PARSE,     // would just return an AST
    CONTEXT_KIND_INTERPRET,
} ContextKind;

/*
 * Compiler Context
 */
typedef struct Context {
    ContextKind kind;
    const char *file;
    
    /* The entire contents of the current translation unit */
    uint8_t *buf;
    
    /* Current position indicators */
    uint8_t *pos;
    uint32_t line;
    
    /* TODO(bloggins): Turn this into a list and control error termination
     * better */
    int last_error;
    
    Scope *active_scope;
    
    /* Parsed AST */
    ASTTopLevel *ast;
} Context;

Scope *context_scope_push(Context *ctx);
Scope *context_scope_pop(Context *ctx);

#endif
