//
//  scope.c
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#pragma mark CT VTABLE Overrides

void Scope_dump(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, FILE *f, Scope *scope) {
    scope_fdump(f, scope);
}

#pragma mark normal functions


Scope *scope_create() {
    Scope *s = ct_create(CT_TYPE_Scope, 0);
    
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
    assert(decl->name && "declaration must have a name");
    assert(AST_BASE(decl)->kind > AST_DECL_BEGIN && AST_BASE(decl)->kind < AST_DECL_END &&
           "not a declaration");
    
    List_FOREACH(ASTDeclaration*, d, scope->declarations, {
        assert(d->name && "declaration must have a name");
        if (spelling_equal(d->name->base.location.spelling,
                           decl->name->base.location.spelling)) {
            SourceLocation *sl = &(AST_BASE(decl)->location);
            Spelling sp = decl->name->base.location.spelling;
            diag_emit(DIAG_ERROR, ERR_ANALYZE, sl, "something named '%s' was "
                        "already declared in this scope", spelling_cstring(sp));
        }
    })
    
    list_append(&scope->declarations, decl);
}

void scope_label_add(Scope *scope, ASTBase *labeled_node) {
    assert(scope && "scope should not be null");
    assert(labeled_node && "label should not be null");
    assert(AST_IS(labeled_node, AST_STMT_LABELED) && "only ASTStmtLabeled type supported");
    
    ASTStmtLabeled *labeled = (ASTStmtLabeled*)labeled_node;
    List_FOREACH(ASTStmtLabeled*, l, scope->labels, {
        if (spelling_equal(AST_BASE(l->label)->location.spelling,
                           labeled->label->base.location.spelling)) {
            SourceLocation *sl = &(AST_BASE(labeled)->location);
            Spelling sp = labeled->label->base.location.spelling;
            diag_emit(DIAG_ERROR, ERR_ANALYZE, sl, "another label named '%s' was "
                        "already declared in this scope", spelling_cstring(sp));
        }
    })
    
    list_append(&scope->labels, labeled);
}

ASTDeclaration *scope_lookup_declaration(Scope *scope, Spelling name, bool search_parents) {
    while(scope != NULL) {
        List_FOREACH(ASTDeclaration *, decl, scope->declarations, {
            if (spelling_equal(name, decl->name->base.location.spelling)) {
                assert(decl->name && "declarations should be named");
                return decl;
            }
        })
    
        if (search_parents) {
            scope = scope->parent;
        } else {
            break;
        }
    }
    
    return NULL;
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
