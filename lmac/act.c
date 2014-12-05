//
//  act.c
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

void act_on_toplevel(Context *ctx, SourceLocation sl, ASTList *stmts) {
    ASTTopLevel *tl = ast_create_toplevel();
    
    tl->base.location = sl;
    tl->definitions = stmts;
    
    ASTLIST_FOREACH(ASTBase*, stmt, stmts, {
        stmt->parent = (ASTBase*)tl;
    });
    
    ctx->ast = tl;
}