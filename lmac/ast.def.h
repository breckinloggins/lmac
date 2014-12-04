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
AST(STMT_RETURN, stmt_return, ASTStmtReturn)
AST(BLOCK, block, ASTBlock)
AST(DEFN_FUNC, defn_func, ASTDefnFunc)
AST(DEFN_VAR, defn_var, ASTDefnVar)
AST(EXPR_IDENT, expr_ident, ASTExprIdent)
AST(EXPR_NUMBER, expr_number, ASTExprNumber)
AST(EXPR_BINARY, expr_binary, ASTExprBinary)
AST(IDENT, ident, ASTIdent)
AST(OPERATOR, operator, ASTOperator)

/* Should ALWAYS be last */
AST(LAST, last, ASTBase)

#ifdef AST
#undef AST
#endif