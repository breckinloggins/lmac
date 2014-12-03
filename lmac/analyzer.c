//
//  analyzer.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

typedef struct {
    ASTList *identifiers;
} AnalyzeCtx;

int ast_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    if (phase == VISIT_PRE) {
        return VISIT_OK;
    }
    
    AnalyzeCtx *actx = (AnalyzeCtx*)ctx;
    
    if (node->kind == AST_EXPR_IDENT) {
        ast_list_add(&actx->identifiers, (ASTBase*)((ASTExprIdent*)node)->name);
    }
    
    return VISIT_OK;
}

ASTBase *nearest_scope_node(ASTBase *node) {
    assert(node && "node should not be null (all nodes should parent to eventual AST_TOPLEVEL");
    if (node->kind == AST_TOPLEVEL || node->kind == AST_BLOCK) {
        return node;
    }
    
    return nearest_scope_node(node->parent);
}

bool spelling_defined_in_scope(Spelling spelling, ASTBase* node) {
    ASTBase *scope = nearest_scope_node(node);

    ASTList *stmts = NULL;
    if (scope->kind == AST_TOPLEVEL) {
        stmts = ((ASTTopLevel*)scope)->definitions;
    } else if (scope->kind == AST_BLOCK) {
        stmts = ((ASTBlock*)scope)->statements;
    }
    
    ASTLIST_FOREACH(ASTBase*, scope_node, stmts, {
        ASTIdent *defn_ident = NULL;
        if (scope_node->kind == AST_DEFN_FUNC) {
            defn_ident = ((ASTDefnFunc*)scope_node)->name;
        } else if (scope_node->kind == AST_DEFN_VAR) {
            defn_ident = ((ASTDefnVar*)scope_node)->name;
        } else {
            continue;
        }
        
        assert(defn_ident && "definitions should always be named");
        if (spelling_equal(spelling, defn_ident->base.location.spelling)) {
            return true;
        }
    })
    
    if (scope->kind == AST_TOPLEVEL) {
        return false;
    }

    return spelling_defined_in_scope(spelling, node->parent);
}

void analyzer_analyze(ASTTopLevel *ast) {
    AnalyzeCtx ctx = {};
    ast_visit((ASTBase*)ast, ast_visitor, &ctx);
    
    ASTLIST_FOREACH(ASTIdent*, ident, ctx.identifiers, {
        if (!spelling_defined_in_scope(ident->base.location.spelling, (ASTBase*)ident)) {
            fprintf(stderr, "error (line %d): undeclared identifier '", ident->base.location.line);
            spelling_fprint(stderr, ident->base.location.spelling);
            fprintf(stderr, "'\n");
            
            exit(ERR_ANALYZE);
        }
    })
}
