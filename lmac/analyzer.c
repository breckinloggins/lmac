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

#define ANALYZE_ERROR(sl, ...)                                              \
    diag_printf(DIAG_ERROR, sl, __VA_ARGS__);                               \
    exit(ERR_ANALYZE);

void check_supported_type(ASTBase *loc_node, Spelling sp_type) {
    bool is_void_type = spelling_streq(sp_type, "void");
    
    if (is_void_type && loc_node->kind == AST_DEFN_VAR) {
        ANALYZE_ERROR(&(loc_node->location),
                      "variables cannot be defined as type 'void'");
    }
}

int ast_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    if (phase == VISIT_PRE) {
        return VISIT_OK;
    }
    
    AnalyzeCtx *actx = (AnalyzeCtx*)ctx;
    
    if (node->kind == AST_EXPR_IDENT) {
        ast_list_add(&actx->identifiers, (ASTBase*)((ASTExprIdent*)node)->name);
    } else if (node->kind == AST_DEFN_FUNC) {
        ASTDefnFunc *func = (ASTDefnFunc*)node;
        Spelling sp_type = AST_BASE(func->type)->location.spelling;
        check_supported_type(node, sp_type);
        
        if (spelling_streq(func->name->base.location.spelling, "main") &&
            !spelling_streq(sp_type, "int")) {
            ANALYZE_ERROR(&(func->base.location), "function main() must have return type 'int'");
        }
        
    } else if (node->kind == AST_DEFN_VAR) {
        ASTDefnVar *var = (ASTDefnVar*)node;
        Spelling sp_type = AST_BASE(var->type)->location.spelling;
        check_supported_type(node, sp_type);
    } else if (ast_node_is_type_expression(node)) {
        Spelling sp_t = node->location.spelling;
        if (!spelling_streq(sp_t, "int") && !spelling_streq(sp_t, "void")) {
            ANALYZE_ERROR(&node->location, "invalid type '%s'", spelling_cstring(sp_t));
        }

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
            ANALYZE_ERROR(&ident->base.location, "undeclared identifier '%s'", spelling_cstring(ident->base.location.spelling));
        }
    })
}
