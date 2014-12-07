//
//  context.c
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

Scope *context_scope_push(Context *ctx) {
    Scope *s = scope_create();
    
    Scope *prev = ctx->active_scope;
    ctx->active_scope = s;
    
    if (prev != NULL) {
        scope_child_add(prev, s);
    }
    
    return s;
}

Scope *context_scope_pop(Context *ctx) {
    Scope *prev = ctx->active_scope;
    assert(prev && "cannot pop empty scope");
    assert(prev->parent && "cannot pop last scope");
    
    ctx->active_scope = prev->parent;
    return prev->parent;
}
