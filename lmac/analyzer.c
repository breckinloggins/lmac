//
//  analyzer.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

typedef struct {
    List *identifiers;
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
        list_append(&actx->identifiers, (ASTBase*)((ASTExprIdent*)node)->name);
    } else if (node->kind == AST_DEFN_FUNC) {
        ASTDefnFunc *func = (ASTDefnFunc*)node;
        ASTTypeExpression *canonical_type = ast_type_get_canonical_type(func->type);
        Spelling sp_type = AST_BASE(canonical_type)->location.spelling;
        check_supported_type(node, sp_type);
        
        if (spelling_streq(func->base.name->base.location.spelling, "main") &&
            !spelling_streq(sp_type, "$32")) {
            // TODO(bloggins): This is temporary and wrong
            ANALYZE_ERROR(&(AST_BASE(func)->location), "function main() must have return type '$32'");
        }
        
    } else if (node->kind == AST_DEFN_VAR) {
        ASTDefnVar *var = (ASTDefnVar*)node;
        ASTTypeExpression *canonical_type = ast_type_get_canonical_type(var->type);
        Spelling sp_type = AST_BASE(canonical_type)->location.spelling;
        check_supported_type(node, sp_type);
    } else if (ast_node_is_type_definition(node)) {
        if (AST_BASE(node)->kind == AST_TYPE_NAME) {
            // Don't need to do anything right now, just don't want to fall to below
        } else if (AST_BASE(node)->kind != AST_TYPE_CONSTANT) {
            // This is just a placeholder so we can catch types we can't codegen yet
            Spelling sp_t = node->location.spelling;
            ANALYZE_ERROR(&node->location, "invalid type '%s' (not supported)", spelling_cstring(sp_t));
        }
    }
    
    return VISIT_OK;
}

void analyzer_analyze(ASTTopLevel *ast) {
    AnalyzeCtx ctx = {};
    ast_visit((ASTBase*)ast, ast_visitor, &ctx);

    List_FOREACH(ASTIdent*, ident, ctx.identifiers, {
        if (ast_nearest_spelling_definition(ident->base.location.spelling, (ASTBase*)ident) == NULL) {
            ANALYZE_ERROR(&ident->base.location, "I don't know what '%s' is", spelling_cstring(ident->base.location.spelling));
        }
    })
    
    ast_visit_data_clean((ASTBase*)ast);
}
