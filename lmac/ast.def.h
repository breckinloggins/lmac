//
//  ast.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef AST
#define AST(ucase_name, name, struct)
#endif

/* Should ALWAYS be first */
AST(UNKNOWN, unknown, ASTBase)

AST(TOPLEVEL, toplevel, ASTTopLevel)
AST(BLOCK, block, ASTBlock)
AST(DEFN_FUNC, defn_func, ASTDefnFunc)
AST(DEFN_VAR, defn_var, ASTDefnVar)
AST(IDENT, ident, ASTIdent)
AST(OPERATOR, operator, ASTOperator)

AST(STMT_RETURN, stmt_return, ASTStmtReturn)
AST(STMT_EXPR, stmt_expr, ASTStmtExpr)

/* Expressions */
AST(EXPR_BEGIN, expr_begin, ASTBase)
AST(EXPR_EMPTY, expr_empty, ASTExprEmpty)
AST(EXPR_NUMBER, expr_number, ASTExprNumber)
AST(EXPR_STRING, expr_string, ASTExprString)
AST(EXPR_IDENT, expr_ident, ASTExprIdent)
AST(EXPR_PAREN, expr_paren, ASTExprParen)
AST(EXPR_CAST, expr_cast, ASTExprCast)
AST(EXPR_BINARY, expr_binary, ASTExprBinary)
AST(EXPR_CALL, expr_call, ASTExprCall)
AST(EXPR_END, expr_end, ASTBase)

/* Preprocessor */
AST(PP_PRAGMA, pp_pragma, ASTPPPragma)
AST(PP_DEFINITION, pp_definition, ASTPPDefinition)
AST(PP_IF, pp_if, ASTPPIf)

/* Type Expressions */
AST(TYPE_BEGIN, type_begin, ASTBase)    /* Type Class Start Guard */
AST(TYPE_CONSTANT, type_constant, ASTTypeConstant)
AST(TYPE_NAME, type_name, ASTTypeName)
AST(TYPE_POINTER, type_pointer, ASTTypePointer)
AST(TYPE_END, type_end, ASTBase)        /* Type Class End Guard */

/* Should ALWAYS be last */
AST(LAST, last, ASTBase)

#ifdef AST
#undef AST
#endif