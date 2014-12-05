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

/*
 * Compiler Context
 */
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

#endif