//
//  type.c
//  lmac
//
//  Created by Breckin Loggins on 12/12/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#include <stdarg.h>

ASTTypeExpression *infer(ASTBase *node);

void type_error(ASTBase *node, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *msg = NULL;
    vasprintf(&msg, fmt, ap);
    va_end(ap);
    
    SourceLocation *sl = NULL;
    if (node != NULL) {
        sl = &node->location;
    }
    
    diag_printf(DIAG_ERROR, sl, "%s", msg);
    free(msg);
    exit(ERR_TYPECHECK);
}

#define TC_INFER_FN(kind, type) ASTTypeExpression *tc_infer_##kind(type *node)

#define QUICK_CHECK if (AST_BASE((node))->inferred_type != NULL) \
    return AST_BASE((node))->inferred_type;

#define CACHE(inferred_t) AST_BASE((node))->inferred_type = (ASTTypeExpression*)(inferred_t)

#pragma Type Expressions

/*
TC_INFER_FN(AST_TYPE_CONSTANT, ASTTypeConstant) {
    QUICK_CHECK
    
    // TODO(bloggins): This is wrong. It should return the kind
    return CACHE(node);
}

TC_INFER_FN(AST_TYPE_NAME, ASTTypeName) {
    QUICK_CHECK
    
    return CACHE(node);
}

TC_INFER_FN(AST_TYPE_POINTER, ASTTypePointer) {
    QUICK_CHECK
    
    return CACHE(node);
}

# pragma Expressions

TC_INFER_FN(AST_EXPR_IDENT, ASTExprIdent) {
    QUICK_CHECK
    
    //return CACHE())
    return NULL;
}
*/

ASTTypeExpression *infer(ASTBase *node) {
#   define TC_DISPATCH_BEGIN(kind)  switch (kind) {
#   define TC_DISPATCH_END()    default: return NULL; }
#   define TC_DISPATCH(kind, type)                          \
case kind: {                                                \
return tc_infer_##kind((type*)node);                        \
} break;
    
    TC_DISPATCH_BEGIN(node->kind)
//    TC_DISPATCH(AST_TOPLEVEL, ASTTopLevel);
//    TC_DISPATCH(AST_DECL_VAR, ASTDeclVar);
//    TC_DISPATCH(AST_DECL_FUNC, ASTDeclFunc);
//    TC_DISPATCH(AST_BLOCK, ASTBlock);
//    TC_DISPATCH(AST_OPERATOR, ASTOperator);
//    TC_DISPATCH(AST_IDENT, ASTIdent);
//    
//    TC_DISPATCH(AST_STMT_RETURN, ASTStmtReturn);
//    TC_DISPATCH(AST_STMT_EXPR, ASTStmtExpr);
//    TC_DISPATCH(AST_STMT_DECL, ASTStmtDecl);
//    TC_DISPATCH(AST_STMT_IF, ASTStmtIf);
//    TC_DISPATCH(AST_STMT_JUMP, ASTStmtJump);
//    TC_DISPATCH(AST_STMT_LABELED, ASTStmtLabeled);
//    
//    TC_DISPATCH(AST_EXPR_BINARY, ASTExprBinary);
//       TC_DISPATCH(AST_EXPR_IDENT, ASTExprIdent);
//    TC_DISPATCH(AST_EXPR_NUMBER, ASTExprNumber);
//    TC_DISPATCH(AST_EXPR_STRING, ASTExprString);
//    TC_DISPATCH(AST_EXPR_CAST, ASTExprCast);
//    TC_DISPATCH(AST_EXPR_PAREN, ASTExprParen);
//    TC_DISPATCH(AST_EXPR_CALL, ASTExprCall);
//    
//       TC_DISPATCH(AST_TYPE_CONSTANT, ASTTypeConstant);
//       TC_DISPATCH(AST_TYPE_NAME, ASTTypeName);
//       TC_DISPATCH(AST_TYPE_POINTER, ASTTypePointer);
//    
//    TC_DISPATCH(AST_PP_PRAGMA, ASTPPPragma);
    TC_DISPATCH_END()
    return VISIT_OK;
}

ASTTypeExpression *type_infer(ASTBase *node) {
    return infer(node);
}