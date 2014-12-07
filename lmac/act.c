//
//  act.c
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

void act_on_pp_pragma(SourceLocation sl, ASTIdent *arg1, ASTIdent *arg2,
                      ASTPPPragma **result) {
    if (result == NULL) return;
    
    ASTPPPragma *pragma = ast_create_pp_pragma();
    AST_BASE(pragma)->location = sl;
    
    // TODO(bloggins): handle arg1
    
    AST_BASE(arg2)->parent = (ASTBase*)pragma;
    pragma->arg = arg2;
    
    *result = pragma;
}

void act_on_toplevel(SourceLocation sl, Scope *scope, List *stmts,
                     ASTTopLevel **result) {
    if (result == NULL) return;
    
    ASTTopLevel *tl = ast_create_toplevel();
    
    tl->base.location = sl;
    tl->base.scope = scope;
    tl->definitions = stmts;
    
    List_FOREACH(ASTBase*, stmt, stmts, {
        stmt->parent = (ASTBase*)tl;
    });
    
    *result = tl;
}

void act_on_defn_var(SourceLocation sl, Scope *scope, ASTTypeExpression*type,
                     ASTIdent *name, ASTExpression *expr, ASTDefnVar **result) {
    if (result == NULL) return;
    
    ASTDefnVar *defn = ast_create_defn_var();
    defn->base.location = sl;
    defn->base.scope = scope;
    AST_BASE(type)->parent = name->base.parent =
        AST_BASE(expr)->parent = (ASTBase*)defn;
    
    defn->type = type;
    defn->name = name;
    defn->expression = expr;
    
    if (scope != NULL) {
        scope_declaration_add(scope, (ASTBase*)defn);
    }
    *result = defn;
}

void act_on_defn_fn(SourceLocation sl, Scope *scope, ASTTypeExpression *type,
                    ASTIdent *name, ASTBlock *block, ASTDefnFunc **result) {
    if (result == NULL) return;
    
    ASTDefnFunc *defn = ast_create_defn_func();
    defn->base.location = sl;
    defn->base.scope = scope;
    block->base.parent = (ASTBase*)defn;
    AST_BASE(type)->parent = name->base.parent = (ASTBase*)defn;
    
    defn->type = type;
    defn->name = name;
    defn->block = block;
    
    if (scope != NULL) {
        scope_declaration_add(scope, (ASTBase*)defn);
    }
    *result = defn;
}

void act_on_stmt_expression(SourceLocation sl, ASTExpression *expr,
                            ASTStmtExpr **result) {
    if (result == NULL) return;
    
    if (expr == NULL) {
        // Expression statements can be empty
        expr = (ASTExpression*)ast_create_expr_empty();
    }
    
    ASTStmtExpr *stmt = ast_create_stmt_expr();
    AST_BASE(expr)->parent = (ASTBase*)stmt;
    
    AST_BASE(stmt)->location = sl;
    stmt->expression = expr;
    
    *result = stmt;
}

void act_on_stmt_return(SourceLocation sl, ASTExpression *expr,
                        ASTStmtReturn **result) {
    if (result == NULL) return;
    
    ASTStmtReturn *stmt = ast_create_stmt_return();
    stmt->base.location = sl;
    AST_BASE(expr)->parent = (ASTBase *)stmt;
    stmt->expression = expr;
    
    *result = stmt;
}

void act_on_block(SourceLocation sl, List *stmts, ASTBlock **result) {
    if (result == NULL) return;
    
    ASTBlock *b = ast_create_block();
    b->base.location = sl;
    
    List_FOREACH(ASTBase*, stmt, stmts, {
        stmt->parent = (ASTBase*)b;
    });
    
    b->statements = stmts;
    
    *result = b;
}

void act_on_type_constant(SourceLocation sl,
                          uint32_t type_id, uint8_t bit_flags, uint64_t bit_size,
                          ASTTypeConstant **result) {
    if (result == NULL) return;
    
    ASTTypeConstant *type = ast_create_type_constant();
    AST_BASE(type)->location = sl;
    type->base.type_id = type_id;
    type->bit_flags = bit_flags;
    type->bit_size = bit_size;
    
    *result = type;
}

void act_on_type_name(SourceLocation sl, ASTIdent *name, ASTTypeName **result) {
    if (result == NULL) return;
    
    ASTTypeName *type_name = ast_create_type_name();
    AST_BASE(type_name)->location = sl;
    AST_BASE(name)->parent = (ASTBase*)type_name;
    
    type_name->name = name;
    
    *result = type_name;
}

void act_on_type_pointer(SourceLocation sl, ASTTypeExpression *pointed_to,
                         ASTTypePointer **result) {
    if (result == NULL) return;
    
    ASTTypePointer *ptr = ast_create_type_pointer();
    AST_BASE(ptr)->location = sl;
    AST_BASE(pointed_to)->parent = (ASTBase*)ptr;
    
    ptr->pointer_to = pointed_to;
    
    *result = ptr;
}

void act_on_expr_ident(SourceLocation sl, ASTExprIdent **result) {
    if (result == NULL) return;
    
    ASTIdent *name = ast_create_ident();
    AST_BASE(name)->location = sl;
    
    ASTExprIdent *ident = ast_create_expr_ident();
    name->base.location = sl;
    name->base.parent = (ASTBase*)ident;
    ident->name = name;
    
    *result = ident;
}

void act_on_expr_number(SourceLocation sl, int n, ASTExprNumber **result) {
    if (result == NULL) return;
    
    ASTExprNumber *number = ast_create_expr_number();
    
    AST_BASE(number)->location = sl;
    number->number = n;
    
    *result = number;
}

void act_on_expr_string(SourceLocation sl, ASTExprString **result) {
    if (result == NULL) return;
    
    ASTExprString *str = ast_create_expr_string();
    
    AST_BASE(str)->location = sl;
    
    *result = str;
}

void act_on_expr_paren(SourceLocation sl, ASTExpression *inner,
                       ASTExprParen **result) {
    if (result == NULL) return;
    
    ASTExprParen *paren = ast_create_expr_paren();
    AST_BASE(paren)->location = sl;
    AST_BASE(inner)->parent = (ASTBase*)paren;
    
    paren->inner = inner;
    
    *result = paren;
}

void act_on_expr_call(SourceLocation sl, ASTExpression *callable,
                      ASTExprCall **result) {
    if (result == NULL) return;
    
    ASTExprCall *call = ast_create_expr_call();
    if (callable != NULL) {
        AST_BASE(callable)->parent = (ASTBase*)call;
    }
    
    AST_BASE(call)->location = sl;
    call->callable = callable;
    
    *result = call;
}

void act_on_expr_cast(SourceLocation sl, ASTTypeExpression *type,
                      ASTExpression *expr, ASTExprCast **result) {
    if (result == NULL) return;
    
    ASTExprCast *cast_expr = ast_create_expr_cast();
    AST_BASE(type)->parent = (ASTBase*)cast_expr;
    AST_BASE(expr)->parent = (ASTBase*)cast_expr;
    AST_BASE(cast_expr)->location = sl;
    cast_expr->type = type;
    cast_expr->expr = expr;
    
    *result = cast_expr;
}

void act_on_expr_binary(SourceLocation sl, ASTExpression *left, ASTExpression *right,
                        Token op, ASTExprBinary **result) {
    if (result == NULL) return;
    
    ASTExprBinary *binop = ast_create_expr_binary();
    ASTOperator *op_node = ast_create_operator();
    op_node->base.location = sl;
    op_node->op = op;
    ast_init_expr_binary(binop, left, right, op_node);
    
    *result = binop;
}

void act_on_ident(SourceLocation sl, ASTIdent **result) {
    if (result == NULL) return;
    
    ASTIdent *ident = ast_create_ident();
    ident->base.location = sl;
    
    *result = ident;
}
