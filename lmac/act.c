//
//  act.c
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

void act_on_pp_pragma(SourceLocation sl, ASTIdent *arg1, ASTIdent *arg2,
                      ASTPPPragma **result) {
    if (result == NULL) return;
    
    ASTPPPragma *pragma = ast_create_pp_pragma();
    AST_BASE(pragma)->location = sl;
    
    // TODO(bloggins): handle arg1
    
    AST_BASE(arg2)->parent = (ASTBase*)pragma;
    pragma->arg = arg2;
    
    *result = pragma;
}

void act_on_toplevel(SourceLocation sl, ASTList *stmts, ASTTopLevel **result) {
    if (result == NULL) return;
    
    ASTTopLevel *tl = ast_create_toplevel();
    
    tl->base.location = sl;
    tl->definitions = stmts;
    
    ASTLIST_FOREACH(ASTBase*, stmt, stmts, {
        stmt->parent = (ASTBase*)tl;
    });
    
    *result = tl;
}

void act_on_defn_var(SourceLocation sl, ASTTypeExpression*type,
                     ASTIdent *name, ASTExpression *expr, ASTDefnVar **result) {
    if (result == NULL) return;
    
    ASTDefnVar *defn = ast_create_defn_var();
    defn->base.location = sl;
    AST_BASE(type)->parent = name->base.parent = AST_BASE(expr)->parent = (ASTBase*)defn;
    
    defn->type = type;
    defn->name = name;
    defn->expression = expr;
    
    *result = defn;
}

void act_on_defn_fn(SourceLocation sl, ASTTypeExpression *type,
                    ASTIdent *name, ASTBlock *block, ASTDefnFunc **result) {
    if (result == NULL) return;
    
    ASTDefnFunc *defn = ast_create_defn_func();
    defn->base.location = sl;
    block->base.parent = (ASTBase*)defn;
    AST_BASE(type)->parent = name->base.parent = (ASTBase*)defn;
    
    defn->type = type;
    defn->name = name;
    defn->block = block;
    
    *result = defn;
}