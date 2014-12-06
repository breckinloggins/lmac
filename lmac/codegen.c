//
//  codegen.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

int cg_visitor(ASTBase *node, VisitPhase phase, void *ctx);

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

#pragma mark Type Expressions

CG_VISIT_FN(AST_TYPE_NAME, ASTTypeName) {
    // type *node, VisitPhase phase, CGContext *ctx
    if (phase == VISIT_POST) {
        assert(false && "should never get to post visit");
    }
    
    if (node->resolved_type == NULL) {
        diag_printf(DIAG_FATAL, &AST_BASE(node)->location, "type name %s "
                    "was never resolved", spelling_cstring(node->name->base.location.spelling));
        exit(ERR_CODEGEN);
    }
    
    ASTTypeExpression *type = ast_type_get_canonical_type(node->resolved_type);
    ast_visit((ASTBase*)type, cg_visitor, ctx);
    
    return VISIT_HANDLED;
}

CG_VISIT_FN(AST_TYPE_CONSTANT, ASTTypeConstant) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (node->base.type_id & TYPE_FLAG_KIND) {
        // We don't directly emit meta type expressions
        return VISIT_OK;
    }
    
    if (phase == VISIT_POST) {
        return VISIT_OK;
    }
    
    //Spelling sp = AST_BASE(node)->location.spelling;
    if (node->base.type_id == 0) {
        CG("void");
    } else {
        if (node->bit_flags & BIT_FLAG_FP) {
            switch (node->bit_size) {
                case 32: CG("float"); break;
                case 64: CG("double"); break;
                default:
                    diag_printf(DIAG_ERROR, &AST_BASE(node)->location,
                                "Cannot generate storage for a floating point "
                                "type of size %d bit",
                                node->bit_size);
                    exit(ERR_CODEGEN);
                    break;
 
            }
        } else {
            if (!(node->bit_flags & BIT_FLAG_SIGNED)) CG("u");
            
            switch (node->bit_size) {
                // TODO(bloggins): Round to nearest power of two for odd sizes
                case 8: CG("int8_t"); break;
                case 16: CG("int16_t"); break;
                case 32: CG("int32_t"); break;
                case 64: CG("int64_t"); break;
                default:
                    diag_printf(DIAG_ERROR, &AST_BASE(node)->location,
                                "Cannot generate storage for a type of size %d bit",
                                node->bit_size);
                    exit(ERR_CODEGEN);
                    break;
            }
        }
    }
    
    return VISIT_OK;
}

#pragma mark Others

CG_VISIT_FN(AST_TOPLEVEL, ASTTopLevel) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CG("// This is a generated file. Do not edit"); CGNL();
        
        // TODO(bloggins): Let's not include anything unless some flag in the
        // context tells us to
        CG("#include <stdint.h>"); CGNL();
        CGNL();
    }
    return VISIT_OK;
}

CG_VISIT_FN(AST_DEFN_VAR, ASTDefnVar) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (node->type->type_id & TYPE_FLAG_KIND) {
        // We don't directly emit meta type expressions
        return VISIT_HANDLED;
    }
    
    if (ast_node_is_type_definition((ASTBase*)node)) {
        // We don't directly emit type definitions
        return VISIT_HANDLED;
    }
    
    if (phase == VISIT_PRE) {
    
    } else {
        CG(";"); CGNL();
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_DEFN_FUNC, ASTDefnFunc) {
    // type *node, VisitPhase phase, CGContext *ctx

    // TODO(bloggins): Remove all of this when we properly handle the subparts
    // of a function definition
    if (phase == VISIT_PRE) {
        ast_visit((ASTBase*)node->type, cg_visitor, ctx);
        CGSPACE();
        CGNODE(node->name); CG("() ");
        ast_visit((ASTBase*)node->block, cg_visitor, ctx);
    }
    
    return VISIT_HANDLED;
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

CG_VISIT_FN(AST_IDENT, ASTIdent) {
    // type *node, VisitPhase phase, CGContext *ctx
    if (phase == VISIT_POST) {
        return VISIT_OK;
    }
    
    if (node->base.parent != NULL && node->base.parent->kind == AST_DEFN_VAR) {
        if (!ast_node_is_type_definition(node->base.parent)) {
            CGSPACE();
            CGNODE(node);
            CGSPACE();
            
            // TODO(bloggins): Major hack. We only do this because we don't
            // currently store the assignment operator in the AST
            CG("="); CGSPACE();
        }
    }
    
    return VISIT_OK;
}

CG_VISIT_FN(AST_OPERATOR, ASTOperator) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        Spelling op_sp = node->op.location.spelling;
        CGSPACE(); CG("%s", spelling_cstring(op_sp)); CGSPACE();
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

CG_VISIT_FN(AST_EXPR_STRING, ASTExprString) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        CG("\"%s\"", spelling_cstring(AST_BASE(node)->location.spelling));
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

CG_VISIT_FN(AST_EXPR_CAST, ASTExprCast) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_POST) {
        assert(false && "should never post visit here");
    }
    
    CG("(");
    ast_visit((ASTBase*)node->type, cg_visitor, ctx);
    CG(")");
    ast_visit((ASTBase*)node->expr, cg_visitor, ctx);
    
    return VISIT_HANDLED;
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

CG_VISIT_FN(AST_EXPR_CALL, ASTExprCall) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        return VISIT_OK;
    }
    
    CG("("); CG(")");
    
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

CG_VISIT_FN(AST_STMT_EXPR, ASTStmtExpr) {
    // type *node, VisitPhase phase, CGContext *ctx
    
    if (phase == VISIT_PRE) {
        return VISIT_OK;
    } else {
        CG(";"); CGNL();
    }
    
    return VISIT_OK;
}

#pragma mark Preprocessor

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

#pragma mark API

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
        CG_DISPATCH(AST_IDENT, ASTIdent);
    
        CG_DISPATCH(AST_STMT_RETURN, ASTStmtReturn);
        CG_DISPATCH(AST_STMT_EXPR, ASTStmtExpr);
    
        CG_DISPATCH(AST_EXPR_BINARY, ASTExprBinary);
        CG_DISPATCH(AST_EXPR_IDENT, ASTExprIdent);
        CG_DISPATCH(AST_EXPR_NUMBER, ASTExprNumber);
        CG_DISPATCH(AST_EXPR_STRING, ASTExprString);
        CG_DISPATCH(AST_EXPR_CAST, ASTExprCast);
        CG_DISPATCH(AST_EXPR_PAREN, ASTExprParen);
        CG_DISPATCH(AST_EXPR_CALL, ASTExprCall);
    
        CG_DISPATCH(AST_TYPE_CONSTANT, ASTTypeConstant);
        CG_DISPATCH(AST_TYPE_NAME, ASTTypeName);
    
        CG_DISPATCH(AST_PP_PRAGMA, ASTPPPragma);
    CG_DISPATCH_END()
    return VISIT_OK;
}

void codegen_generate(FILE *f, ASTTopLevel *ast) {
    CGContext cgctx = {};
    cgctx.f = f;
    
    ast_visit((ASTBase*)ast, cg_visitor, &cgctx);
}
