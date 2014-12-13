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
    
    if (is_void_type && AST_IS(loc_node, AST_DECL_VAR)) {
        ANALYZE_ERROR(&(loc_node->location),
                      "variables cannot be defined as type 'void'");
    }
}

int ast_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    if (phase == VISIT_PRE) {
        assert(node->magic == AST_MAGIC);
        return VISIT_OK;
    }
    
    AnalyzeCtx *actx = (AnalyzeCtx*)ctx;
    
    // TODO(bloggins): It's about time to break this up into a dispatch structure
    if (AST_IS(node, AST_EXPR_IDENT)) {
        list_append(&actx->identifiers, (ASTBase*)((ASTExprIdent*)node)->name);
    } else if (AST_IS(node, AST_DECL_FUNC)) {
        ASTDeclFunc *func = (ASTDeclFunc*)node;
        ASTTypeExpression *canonical_type = ast_type_get_canonical_type(func->type);
        Spelling sp_type = AST_BASE(canonical_type)->location.spelling;
        check_supported_type(node, sp_type);
        
        if (spelling_streq(func->base.name->base.location.spelling, "main") &&
            !spelling_streq(sp_type, "$i32")) {
            // TODO(bloggins): This is temporary and wrong
            ANALYZE_ERROR(&(AST_BASE(func)->location), "function main() must have return type '$i32'");
        }
        
    } else if (AST_IS(node, AST_DECL_VAR)) {
        ASTDeclVar *var = (ASTDeclVar*)node;
        ASTTypeExpression *canonical_type = ast_type_get_canonical_type(var->type);
        if (canonical_type != NULL) {
            Spelling sp_type = AST_BASE(canonical_type)->location.spelling;
            check_supported_type(node, sp_type);
        }
    } else if (ast_node_is_type_definition(node)) {
        if (AST_IS(node, AST_TYPE_NAME)) {
            // Don't need to do anything right now, just don't want to fall to below
        } else if (!AST_IS(node, AST_TYPE_CONSTANT)) {
            // This is just a placeholder so we can catch types we can't codegen yet
            Spelling sp_t = node->location.spelling;
            ANALYZE_ERROR(&node->location, "invalid type '%s' (not supported)", spelling_cstring(sp_t));
        }
    } else if (AST_IS(node, AST_STMT_JUMP)) {
        ASTStmtJump *stmt_jump = (ASTStmtJump*)node;
        if (stmt_jump->label != NULL) {
            list_append(&actx->identifiers, stmt_jump->label);
        }
        switch (stmt_jump->keyword.kind) {
            case TOK_KW_CONTINUE:
            case TOK_KW_BREAK:
                if (stmt_jump->label != NULL) {
                    ANALYZE_ERROR(&node->location, "nonlocal '%s' is not supported",
                                  spelling_cstring(stmt_jump->keyword.location.spelling));
                }
                break;
            case TOK_KW_GOTO:
                if (stmt_jump->label == NULL) {
                    ANALYZE_ERROR(&node->location, "goto where? (goto requires a destination label)");
                }
                break;
            default:
                assert(false && "Unhandled case");
        }
    }
    
    assert(node);
    assert(node->magic == AST_MAGIC);
    assert(node->kind > AST_UNKNOWN && node->kind < AST_LAST);
    
    return VISIT_OK;
}

void analyzer_analyze(ASTBase *ast) {
    AnalyzeCtx ctx = {};
    ast_visit((ASTBase*)ast, ast_visitor, &ctx);

    List_FOREACH(ASTIdent*, ident, ctx.identifiers, {
        if (ast_nearest_spelling_definition(ident->base.location.spelling, (ASTBase*)ident) == NULL) {
            if (ast_ident_find_label(ident)) {
                if (AST_IS(AST_BASE(ident)->parent, AST_STMT_JUMP)) {
                    continue;
                }
            }
            
            ANALYZE_ERROR(&ident->base.location, "I don't know what '%s' is", spelling_cstring(ident->base.location.spelling));
        }
    })
    
    ast_visit_data_clean(ast);
}
