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

void scope_declaration_add(Scope *scope, ASTBase *decl) {
    assert(scope && "scope should not be null");
    assert(decl && "declaration should not be null");
    
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
