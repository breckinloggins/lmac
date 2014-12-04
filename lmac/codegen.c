//
//  codegen.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#define CODGEGEN_FATAL(...) diag_printf(DIAG_FATAL, NULL, __VA_ARGS__); \
                            exit(ERR_CODEGEN);

// Codegen pragmas we respect
#define FCG_EXPLICIT_PARENS "fcg_explicit_parens"

typedef struct {
    FILE *f;
    bool explicit_parens;
} CGContext;

#define CG_VISIT_FN(kind, type) int cg_visit_##kind(type *node, VisitPhase phase, CGContext *ctx)

#define CG(...) fprintf(ctx->f, __VA_ARGS__)
#define CGNL() fprintf(ctx->f, "\n")
#define CGSPACE() fprintf(ctx->f, " ")
#define CGSP(spelling) fprintf(ctx->f, "%s", spelling_cstring((spelling)))
#define CGNODE(node) fprintf(ctx->f, "%s", spelling_cstring(((ASTBase*)(node))->location.spelling))

CG_VISIT_FN(AST_TOPLEVEL, ASTTopLevel) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CG("// This is a generated file. Do not edit"); CGNL();
        CGNL();
    }
    return VISIT_OK;
}

CG_VISIT_FN(AST_DEFN_VAR, ASTDefnVar) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CGNODE(node->type); CGSPACE();
        CGNODE(node->name); CGSPACE();
        CG("="); CGSPACE();
    } else {
        CG(";"); CGNL();
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_DEFN_FUNC, ASTDefnFunc) {
    // type *node, VisitPhase phase, CGContext *ctx

    if (phase == VISIT_PRE) {
        CGNODE(node->type); CGSPACE();
        CGNODE(node->name); CG("() ");
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_BLOCK, ASTBlock) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CG("{"); CGNL();
    } else {
        CG("}"); CGNL();
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_OPERATOR, ASTOperator) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CGSPACE(); CG("%c", node->op); CGSPACE();
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_EXPR_IDENT, ASTExprIdent) {
    // type *node, VisitPhase phase, CGContext *ctx
    if (phase == VISIT_PRE) {
        CGNODE(node->name);
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_EXPR_NUMBER, ASTExprNumber) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CG("%d", node->number);
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_EXPR_PAREN, ASTExprParen) {
    // type *node, VisitPhase phase, CGContext *ctx

    if (phase == VISIT_PRE) {
        CG("(");
    } else {
        CG(")");
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_EXPR_BINARY, ASTExprBinary) {
    // type *node, VisitPhase phase, CGContext *ctx
    if (!ctx->explicit_parens) {
        // Nothing to do in this case. Our children will construct
        // themselves correctly
        return VISIT_OK;
    }
    
    if (phase == VISIT_PRE) {
        CG("(");
    } else {
        CG(")");
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_STMT_RETURN, ASTStmtReturn) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CG("return ");
    } else {
        CG(";"); CGNL();
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_PP_PRAGMA, ASTPPPragma) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_POST) {
        return VISIT_OK;
    }
    
    Spelling arg_sp = node->arg->base.location.spelling;
    if (spelling_streq(arg_sp, FCG_EXPLICIT_PARENS)) {
        ctx->explicit_parens = true;
    } else {
        diag_printf(DIAG_WARNING, &node->arg->base.location,
                    "Unrecognized pragma '%s'. Supported pragmas: "
                    "[" FCG_EXPLICIT_PARENS "]", spelling_cstring(arg_sp));
    }
    
    CG("/* #pragma CLITE %s */", spelling_cstring(arg_sp)); CGNL();
    
    return VISIT_OK;
}

int cg_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
#   define CG_DISPATCH_BEGIN(kind)  switch (kind) {
#   define CG_DISPATCH_END()    default: break; }
#   define CG_DISPATCH(kind, type)                                      \
        case kind: {                                                    \
            CGContext *cgctx = (CGContext*)ctx;                         \
            return cg_visit_##kind((type*)node, phase, cgctx);          \
        } break;

    CG_DISPATCH_BEGIN(node->kind)
        CG_DISPATCH(AST_TOPLEVEL, ASTTopLevel);
        CG_DISPATCH(AST_DEFN_VAR, ASTDefnVar);
        CG_DISPATCH(AST_DEFN_FUNC, ASTDefnFunc);
        CG_DISPATCH(AST_BLOCK, ASTBlock);
        CG_DISPATCH(AST_OPERATOR, ASTOperator);
        CG_DISPATCH(AST_EXPR_BINARY, ASTExprBinary);
        CG_DISPATCH(AST_EXPR_IDENT, ASTExprIdent);
        CG_DISPATCH(AST_EXPR_NUMBER, ASTExprNumber);
        CG_DISPATCH(AST_EXPR_PAREN, ASTExprParen);
        CG_DISPATCH(AST_STMT_RETURN, ASTStmtReturn);
    
        CG_DISPATCH(AST_PP_PRAGMA, ASTPPPragma);
    CG_DISPATCH_END()
    return VISIT_OK;
}

void codegen_generate(FILE *f, ASTTopLevel *ast) {
    CGContext cgctx = {};
    cgctx.f = f;
    
    ast_visit((ASTBase*)ast, cg_visitor, &cgctx);
}
