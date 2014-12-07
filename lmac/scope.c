//
//  scope.c
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

Scope *scope_create() {
    Scope *s = (Scope*)calloc(1, sizeof(Scope));
    
    return s;
}

void scope_child_add(Scope *scope, Scope *child) {
    assert(scope && "scope should not be null");
    assert(child && "child should not be null");
    assert((child->parent == NULL) && "child scope must not already be attached");
    
    list_append(&scope->children, child);
    child->parent = scope;
}

void scope_declaration_add(Scope *scope, ASTDeclaration *decl) {
    assert(scope && "scope should not be null");
    assert(decl && "declaration should not be null");
    
    List_FOREACH(ASTDeclaration*, d, scope->declarations, {
        if (spelling_equal(d->name->base.location.spelling,
                           decl->name->base.location.spelling)) {
            SourceLocation *sl = &(AST_BASE(decl)->location);
            Spelling sp = decl->name->base.location.spelling;
            diag_printf(DIAG_ERROR, sl, "something named '%s' was "
                        "already declared in this scope", spelling_cstring(sp));
            exit(ERR_ANALYZE);
        }
    })
    
    list_append(&scope->declarations, decl);
}

void scope_dump(Scope *scope) {
    scope_fdump(stderr, scope);
}

void scope_fdump(FILE *f, Scope *scope) {
    fprintf(f, "{\n");
    List_FOREACH(ASTBase*, decl, scope->declarations, {
        ast_fprint(f, decl, 1);
    })
    fprintf(f, "}\n");
}
