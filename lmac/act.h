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

// The signature of most functions in the parser
typedef bool (*ParseFn)(Context *ctx, ASTBase **result);

#pragma mark Preprocessor

void act_on_pp_run(SourceLocation sl, Context *ctx, Token chunk, char chunk_escape,
                   ParseFn parser, ASTBase **result);

void act_on_pp_pragma(SourceLocation sl, ASTIdent *arg1, ASTIdent *arg2, Token rest,
                      ASTPPPragma **result);

void act_on_pp_include(SourceLocation sl, const char *include_file, bool system_include,
                       Scope *scope, ASTBase **result);

void act_on_pp_define(SourceLocation sl, ASTIdent *name, Spelling value,
                      ASTPPDefinition **result);

void act_on_pp_ifndef(SourceLocation sl, ASTIdent *ident, ASTPPIf **result);

#pragma mark Toplevel

void act_on_toplevel(SourceLocation sl, Scope *scope, List *stmts, ASTTopLevel **result);

#pragma mark Declarations

void act_on_decl_var(SourceLocation sl, Scope *scope, ASTTypeExpression *type, bool is_const,
                     ASTIdent *name, ASTExpression *expr, ASTDeclVar **result);

void act_on_decl_fn(SourceLocation sl, Scope *scope, ASTTypeExpression *type,
                     ASTIdent *name, List *params, bool has_vararg_param,
                    ASTBlock *block, ASTDeclFunc **result);

#pragma mark Statements

void act_on_block(SourceLocation sl, List *stmts, ASTBlock **result);

void act_on_stmt_expression(SourceLocation sl, ASTExpression *expr,
                            ASTStmtExpr **result);

void act_on_stmt_declaration(SourceLocation sl, ASTDeclaration *decl, ASTStmtDecl **result);

void act_on_stmt_return(SourceLocation sl, ASTExpression *expr,
                            ASTStmtReturn **result);

void act_on_stmt_if(SourceLocation sl, ASTExpression *condition,
                    ASTBase *stmt_true, ASTBase *stmt_false, ASTStmtIf **result);

void act_on_stmt_jump(SourceLocation sl, Token keyword, ASTIdent *label,
                      ASTStmtJump **result);

#pragma mark Type Expressions

void act_on_type_constant(SourceLocation sl,
                          uint32_t type_id, uint8_t bit_flags, uint64_t bit_size,
                          ASTTypeConstant **result);

void act_on_type_name(SourceLocation sl, ASTIdent *name, ASTTypeName **result);

void act_on_type_pointer(SourceLocation sl, ASTTypeExpression *pointed_to,
                         ASTTypePointer **result);

#pragma mark Expressions

void act_on_expr_ident(SourceLocation sl, ASTExprIdent **result);

void act_on_expr_number(SourceLocation sl, int number, ASTExprNumber **result);

void act_on_expr_string(SourceLocation sl, ASTExprString **result);

void act_on_expr_paren(SourceLocation sl, ASTExpression *inner,
                       ASTExprParen **result);

void act_on_expr_call(SourceLocation sl, ASTExpression *callable, List *args,
                      ASTExprCall **result);

void act_on_expr_cast(SourceLocation sl, ASTTypeExpression *type,
                      ASTExpression *expr, ASTExprCast **result);

void act_on_expr_binary(SourceLocation sl, ASTExpression *left, ASTExpression *right,
                        Token op, ASTExprBinary **result);

void act_on_ident(SourceLocation sl, ASTIdent **result);
#endif
