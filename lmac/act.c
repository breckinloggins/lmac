//
//  act.c
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"
#include <unistd.h>

#pragma mark Preprocessor

void act_on_pp_run(SourceLocation sl, Context *ctx, Token chunk, char chunk_escape,
                   ParseFn parser, ASTBase **result) {
    if (result == NULL) return;
    
    Spelling chunk_sp = chunk.location.spelling;
    const char *chunk_src = spelling_cstring(chunk_sp);
    char *chunk_processed = calloc(1, (strlen(chunk_src) + 1) * sizeof(char));
    const char *p = chunk_src;
    int idx = 0;
    while (*p != 0) {
        if (*p != chunk_escape && p != chunk_src) {
            // Skipped over any escape as well as the first character in
            // the chunk (which is a space)
            chunk_processed[idx++] = *p;
        }
        
        p++;
    }
    
    // We want to chop off the end of the chunk at the end-of-chunk marker,
    // which will be the last character processed
    if (idx > 0) {
        idx--;
    }
    chunk_processed[idx] = 0;
    
    // TODO(bloggins): Fork/join here
    Context run_ctx = *ctx;
    run_ctx.ast = NULL;
    run_ctx.buf = (uint8_t*)chunk_processed;
    run_ctx.buf_size = idx;
    run_ctx.pos = run_ctx.buf;
    
    if (parser(&run_ctx, &(run_ctx.ast))) {
        analyzer_analyze(run_ctx.ast);
    }
    
    /*
    ASTBase *run_result = NULL;
    interp_interpret(run_ctx.ast, &run_result);
    
    *result = run_result;
    */
    
    *result = run_ctx.ast;
     
    free(chunk_processed);
}

void act_on_pp_pragma(SourceLocation sl, ASTIdent *arg1, ASTIdent *arg2, Token rest,
                      ASTPPPragma **result) {
    if (result == NULL) return;
    
    ASTPPPragma *pragma = ast_create_pp_pragma();
    AST_BASE(pragma)->location = sl;
    
    // TODO(bloggins): handle arg1
    
    AST_BASE(arg2)->parent = (ASTBase*)pragma;
    pragma->arg = arg2;
    pragma->rest = rest.location.spelling;
    
    if (spelling_streq(pragma->arg->base.location.spelling, "system_header_path")) {
        char *header_path = strdup(spelling_cstring(pragma->rest));
        
        // TODO(bloggins): Pull out into a strip routine
        while (*header_path != '"') {
            if (*header_path == 0) {
                diag_printf(DIAG_ERROR, &rest.location, "invalid header path");
                exit(ERR_PARSE);
            }
            ++header_path;
        }
        ++header_path;
        
        char *hp_end = header_path;
        while (*hp_end != '"') {
            if (*hp_end == 0) {
                diag_printf(DIAG_ERROR, &rest.location, "invalid header path");
                exit(ERR_PARSE);
            }
            ++hp_end;
        }
        *hp_end = 0;
        
        list_append(&(sl.ctx->system_header_paths), header_path);

    }

    *result = pragma;
}

void act_on_pp_include(SourceLocation sl, const char *include_file, bool system_include,
                       Scope *scope, ASTBase **result) {
    if (result == NULL) return;
    
    Context ctx = *sl.ctx;
    ctx.file = NULL;
    ctx.active_scope = NULL;
    
    char *full_path = NULL;
    if (system_include && include_file[0] != '/') {
        // Try to find the system header
        List_FOREACH(char *, hpath, sl.ctx->system_header_paths, {
            asprintf(&full_path, "%s/%s", hpath, include_file);
            if (access(full_path, R_OK) == -1) {
                free(full_path);
                full_path = NULL;
                continue;
            }
        })
    }
    
    if (full_path != NULL) {
        context_load_file(&ctx, full_path);
    } else {
        context_load_file(&ctx, include_file);
    }
    
    context_scope_push(&ctx);
    parser_parse(&ctx);
    
    *result = ctx.ast;
}

void act_on_pp_define(SourceLocation sl, ASTIdent *name, Spelling value,
                      ASTPPDefinition **result) {
    if (result == NULL) return;
    
    ASTPPDefinition *pp_defn = ast_create_pp_definition();
    AST_BASE(pp_defn)->location = sl;
    AST_BASE(name)->parent = (ASTBase*)pp_defn;
    pp_defn->name = name;
    pp_defn->value = value;
    
    list_append(&sl.ctx->pp_defines, pp_defn);
    
    *result = pp_defn;
}

void act_on_pp_ifndef(SourceLocation sl, ASTIdent *ident, ASTPPIf **result) {
    if (result == NULL) return;
    
    ASTPPIf *pp_if = ast_create_pp_if();
    AST_BASE(pp_if)->location = sl;
    pp_if->kind = PP_IF_IFNDEF;
    // pp_if->ident = ident;
    // sl.ctx->lex_mode = LEX_PP_ONLY;
    
    *result = pp_if;
}

#pragma mark Toplevel

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

#pragma mark Declarations

void act_on_decl_var(SourceLocation sl, Scope *scope, ASTTypeExpression*type, bool is_const,
                     ASTIdent *name, ASTExpression *expr, ASTDeclVar **result) {
    if (result == NULL) return;
    
    ASTDeclVar *decl = ast_create_decl_var();
    AST_BASE(decl)->location = sl;
    AST_BASE(decl)->scope = scope;
    AST_BASE(type)->parent = name->base.parent = (ASTBase*)decl;
    
    if (expr != NULL) {
        AST_BASE(expr)->parent = (ASTBase*)decl;
    }
    
    decl->type = type;
    decl->is_const = is_const;
    decl->base.name = name;
    decl->expression = expr;
    
    if (scope != NULL) {
        scope_declaration_add(scope, (ASTDeclaration*)decl);
    }
    
    *result = decl;
}

void act_on_decl_fn(SourceLocation sl, Scope *scope, ASTTypeExpression *type,
                    ASTIdent *name, List *params, bool has_vararg_param,
                    ASTBlock *block, ASTDeclFunc **result) {
    if (result == NULL) return;
    
    ASTDeclFunc *decl = ast_create_decl_func();
    AST_BASE(decl)->location = sl;
    AST_BASE(decl)->scope = scope;
    AST_BASE(type)->parent = name->base.parent = (ASTBase*)decl;
    
    if (params != NULL) {
        List_FOREACH(ASTDeclaration *, param, params, {
            AST_BASE(param)->parent = (ASTBase*)decl;
        });
    }
    
    if (block != NULL) {
        AST_BASE(block)->parent = (ASTBase*)decl;
    }
    
    decl->type = type;
    decl->base.name = name;
    decl->params = params;
    decl->has_varargs = has_vararg_param;
    decl->block = block;
    
    if (scope != NULL) {
        scope_declaration_add(scope, (ASTDeclaration*)decl);
    }
    *result = decl;
}

#pragma mark Statements

void act_on_block(SourceLocation sl, List *stmts, ASTBlock **result) {
    if (result == NULL) return;
    
    ASTBlock *b = ast_create_block();
    AST_BASE(b)->location = sl;
    
    List_FOREACH(ASTBase*, stmt, stmts, {
        stmt->parent = (ASTBase*)b;
    });
    
    b->statements = stmts;
    
    *result = b;
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

void act_on_stmt_declaration(SourceLocation sl, ASTDeclaration *decl,
                             ASTStmtDecl **result) {
    if (result == NULL) return;
    
    assert(decl && "must have valid declaration");
    
    ASTStmtDecl *stmt = ast_create_stmt_decl();
    AST_BASE(decl)->parent = (ASTBase*)stmt;
    
    AST_BASE(stmt)->location = sl;
    stmt->declaration = decl;
    
    *result = stmt;
 
}

void act_on_stmt_return(SourceLocation sl, ASTExpression *expr,
                        ASTStmtReturn **result) {
    if (result == NULL) return;
    
    ASTStmtReturn *stmt = ast_create_stmt_return();
    AST_BASE(stmt)->location = sl;
    AST_BASE(expr)->parent = (ASTBase *)stmt;
    stmt->expression = expr;
    
    *result = stmt;
}

void act_on_stmt_if(SourceLocation sl, ASTExpression *condition,
                    ASTBase *stmt_true, ASTBase *stmt_false, ASTStmtIf **result) {
    if (result == NULL) return;
    
    ASTStmtIf *stmt_if = ast_create_stmt_if();
    AST_BASE(stmt_if)->scope = sl.ctx->active_scope;
    AST_BASE(stmt_if)->location = sl;
    
    AST_BASE(condition)->parent = (ASTBase*)stmt_if;
    stmt_if->condition = condition;
    
    if (stmt_true != NULL) {
        AST_BASE(stmt_true)->parent = (ASTBase*)stmt_if;
        stmt_if->stmt_true = stmt_true;
    }
    
    if (stmt_false != NULL) {
        AST_BASE(stmt_false)->parent = (ASTBase*)stmt_if;
        stmt_if->stmt_false = stmt_false;
    }
    
    *result = stmt_if;
}

void act_on_stmt_jump(SourceLocation sl, Token keyword, ASTIdent *label,
                      ASTStmtJump **result) {
    if (result == NULL) return;
    
    ASTStmtJump *stmt_jump = ast_create_stmt_jump();
    AST_BASE(stmt_jump)->location = sl;
    
    if (label != NULL) {
        AST_BASE(label)->parent = (ASTBase*)stmt_jump;
        AST_BASE(label)->scope = sl.ctx->active_scope;
    }
    
    stmt_jump->keyword = keyword;
    stmt_jump->label = label;
    
    *result = stmt_jump;
}

void act_on_stmt_labeled(SourceLocation sl, ASTIdent *label, ASTStatement *stmt, ASTStmtLabeled **result) {
    if (result == NULL) return;
    
    ASTStmtLabeled *stmt_l = ast_create_stmt_labeled();
    AST_BASE(stmt_l)->location = sl;
    AST_BASE(label)->parent = (ASTBase*)stmt_l;
    AST_BASE(stmt)->parent = (ASTBase*)stmt_l;
    
    stmt_l->label = label;
    stmt_l->stmt = stmt;
    if (sl.ctx && sl.ctx->active_scope) {
        scope_label_add(sl.ctx->active_scope, (ASTBase*)stmt_l);
    }
    
    *result = stmt_l;
}

#pragma mark Type Expressions

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

#pragma mark Expressions

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

void act_on_expr_call(SourceLocation sl, ASTExpression *callable, List *args,
                      ASTExprCall **result) {
    if (result == NULL) return;
    
    ASTExprCall *call = ast_create_expr_call();
    if (callable != NULL) {
        AST_BASE(callable)->parent = (ASTBase*)call;
    }
    
    List_FOREACH(ASTExpression*, arg, args, {
        AST_BASE(arg)->parent = (ASTBase*)call;
        
        // TODO(bloggins): Should we pass scope in?
        AST_BASE(arg)->scope = sl.ctx->active_scope;
    })
    
    AST_BASE(call)->location = sl;
    call->callable = callable;
    call->args = args;
    
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
