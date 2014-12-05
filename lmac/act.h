//
//  act.h
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// Parser actions

#ifndef lmac_act_h
#define lmac_act_h

#include "token.h"
#include "ast.h"
#include "context.h"

void act_on_pp_pragma(SourceLocation sl, ASTIdent *arg1, ASTIdent *arg2,
                      ASTPPPragma **result);

void act_on_toplevel(SourceLocation sl, ASTList *stmts, ASTTopLevel **result);

void act_on_defn_var(SourceLocation sl, ASTTypeExpression *type,
                     ASTIdent *name, ASTExpression *expr, ASTDefnVar **result);

void act_on_defn_fn(SourceLocation sl, ASTTypeExpression *type,
                     ASTIdent *name, ASTBlock *block, ASTDefnFunc **result);

void act_on_stmt_expression(SourceLocation sl, ASTExpression *expr,
                            ASTStmtExpr **result);

void act_on_stmt_return(SourceLocation sl, ASTExpression *expr,
                            ASTStmtReturn **result);

void act_on_block(SourceLocation sl, ASTList *stmts, ASTBlock **result);

void act_on_type_constant(SourceLocation sl,
                          uint32_t type_id, uint8_t bit_flags, uint64_t bit_size,
                          ASTTypeConstant **result);

#endif
