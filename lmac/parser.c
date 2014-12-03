//
//  parser.c
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#define IS_TOKEN_NONE(t) ((t).kind == TOK_NONE)

Context snapshot(Context *ctx) {
    Context saved = *ctx;
    
    return saved;
}

void restore(Context *ctx, Context snapshot) {
    *ctx = snapshot;
}

SourceLocation parsed_source_location(Context *ctx, Context snapshot) {
    SourceLocation sl = {};
    sl.file = ctx->file;
    sl.line = snapshot.line;
    sl.range_start = snapshot.pos;
    sl.range_end = ctx->pos;
    
    return sl;
}

Token next_token(Context *ctx) {
    // TODO(bloggins): figure out how to incorporate comments and ws in generated
    // code
    for (;;) {
        Token t = lexer_next_token(ctx);
        if (t.kind == TOK_WS || t.kind == TOK_COMMENT) {
            continue;
        }
        
        if (t.kind == TOK_UNKOWN) {
            diag_printf(DIAG_FATAL, NULL, "unknown token '%s'", spelling_cstring(t.location.spelling));
            exit(ERR_LEX);
        }
        
        return t;
    }
}

Token peek_token(Context *ctx) {
    // TODO(bloggins): This is probably expensive
    Context s = snapshot(ctx);
    
    Token t = next_token(ctx);
    
    restore(ctx, s);
    return t;
}

Token accept_token(Context *ctx, TokenKind kind) {
    Token t = next_token(ctx);
    if (t.kind == kind) {
        return t;
    }
    
    lexer_put_back(ctx, t);
    
    return TOKEN_NONE;
}

Token expect_token(Context *ctx, TokenKind kind) {
    Token t = accept_token(ctx, kind);
    
    if (IS_TOKEN_NONE(t)) {
        SourceLocation sl = parsed_source_location(ctx, *ctx);
        diag_printf(DIAG_ERROR, &sl, "expected %s", token_get_name(kind));
        exit(ERR_PARSE);
    }
    
    return t;
}

ASTIdent *parse_ident(Context *ctx) {
    Context s = snapshot(ctx);
    
    Token t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    ASTIdent *ident = ast_create_ident();
    ident->base.location = t.location;
    return ident;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

// TODO(bloggins): We need an ASTExpression union
ASTBase *parse_expression(Context *ctx) {
    Context s = snapshot(ctx);
    
    ASTBase *expr = NULL;
    Token t = accept_token(ctx, TOK_IDENT);
    if (!IS_TOKEN_NONE(t)) {
        ASTExprIdent *ident = ast_create_expr_ident();
        ASTIdent *name = ast_create_ident();
        name->base.location = t.location;
        name->base.parent = (ASTBase*)ident;
        ident->name = name;
        
        expr = (ASTBase*)ident;
    } else {
        t = accept_token(ctx, TOK_NUMBER);
        if (!IS_TOKEN_NONE(t)) {
            ASTExprNumber *number = ast_create_expr_number();
            number->base.location = t.location;
            number->number = (int)strtol((char*)t.location.range_start, NULL, 10);
            
            expr = (ASTBase*)number;
        } else {
            goto fail_parse;
        }
    }
    
    // TODO(bloggins): This is a bit of a hack until we get a true left-factored
    //                 grammar parse
    t = peek_token(ctx);
    if (t.kind == TOK_PLUS) {
        // Assume we have a binary expression
        next_token(ctx);  // gobble gobble
        
        ASTBase *left = expr;
        ASTBase *right = parse_expression(ctx);
        if (right == NULL) {
            SourceLocation sl = parsed_source_location(ctx, s);
            diag_printf(DIAG_ERROR, &sl, "expected expression after '+'");
            exit(ERR_PARSE);
        }
        
        ASTExprBinary *binop = ast_create_expr_binary();
        left->parent = right->parent = (ASTBase*)binop;
        
        binop->left = left;
        binop->right = right;
        binop->op = '+';
        
        expr = (ASTBase*)binop;
    }
    
    expr->location = parsed_source_location(ctx, s);
    return expr;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

ASTDefnVar *parse_defn_var(Context *ctx) {
    // TODO(bloggins): Snapshotting works but can be slow (because we might
    //                  backtrack a long way). Should we left-factor instead?
    Context s = snapshot(ctx);
    
    ASTIdent *type = parse_ident(ctx);
    if (type == NULL) { goto fail_parse; }
    
    ASTIdent *name = parse_ident(ctx);
    if (name == NULL) { goto fail_parse; }
    
    Token t = accept_token(ctx, TOK_EQUALS);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    ASTBase *expr = parse_expression(ctx);
    if (expr == NULL) { goto fail_parse; }
    
    t = expect_token(ctx, TOK_SEMICOLON);
    
    ASTDefnVar *defn = ast_create_defn_var();
    defn->base.location = parsed_source_location(ctx, s);
    type->base.parent = name->base.parent = expr->parent = (ASTBase*)defn;
    
    defn->type = type;
    defn->name = name;
    defn->expression = expr;
    
    return defn;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

ASTStmtReturn *parse_stmt_return(Context *ctx) {
    Context s = snapshot(ctx);
    
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_KW_RETURN))) { goto fail_parse; }
    
    ASTBase *expr = parse_expression(ctx);
    if (expr == NULL) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "expected expression");
        exit(ERR_PARSE);
    }
    
    expect_token(ctx, TOK_SEMICOLON);
    
    ASTStmtReturn *stmt = ast_create_stmt_return();
    stmt->base.location = parsed_source_location(ctx, s);
    expr->parent = (ASTBase *)stmt;
    
    stmt->expression = expr;
    
    return stmt;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

ASTBase *parse_block_stmt(Context *ctx) {
    // NOTE(bloggins): When a parse function just switches
    //                  on other parse functions, there's no
    //                  need to save the context ourselves
    
    ASTBase *stmt = (ASTBase *)parse_defn_var(ctx);
    if (stmt == NULL) {
        stmt = (ASTBase*)parse_stmt_return(ctx);
    }
    
    return stmt;
}

ASTBlock *parse_block(Context *ctx) {
    Context s = snapshot(ctx);
    ASTList *stmts = NULL;
    
    Token t = accept_token(ctx, TOK_LBRACE);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    ASTBlock *b = ast_create_block();
    ASTBase *stmt = NULL;
    while ((stmt = (ASTBase*)parse_block_stmt(ctx)) != NULL) {
        stmt->parent = (ASTBase*)b;
        ast_list_add(&stmts, stmt);
    }
    
    t = expect_token(ctx, TOK_RBRACE);
    
    b->base.location = parsed_source_location(ctx, s);
    b->statements = stmts;
    
    return b;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

ASTDefnFunc *parse_defn_fn(Context *ctx) {
    Context s = snapshot(ctx);

    // TODO(bloggins): Factor this grammar into reusable chunks like
    //                  "parse_begin_decl"
    ASTIdent *type = parse_ident(ctx);
    if (type == NULL) { goto fail_parse; }
    
    ASTIdent *name = parse_ident(ctx);
    if (type == NULL) { goto fail_parse; }
    
    Token t = accept_token(ctx, TOK_LPAREN);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    t = accept_token(ctx, TOK_RPAREN);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    ASTBlock *block = parse_block(ctx);
    if (block == NULL) { goto fail_parse; }
    
    ASTDefnFunc *defn = ast_create_defn_func();
    defn->base.location = parsed_source_location(ctx, s);
    block->base.parent = (ASTBase*)defn;
    type->base.parent = name->base.parent = (ASTBase*)defn;
    
    defn->type = type;
    defn->name = name;
    defn->block = block;
    return defn;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

void parse_end(Context *ctx) {
    expect_token(ctx, TOK_END);
}

ASTTopLevel *parse_toplevel(Context *ctx) {
    Context s = snapshot(ctx);
    ASTList *defns = NULL;
    
    ASTTopLevel *tl = ast_create_toplevel();
    ASTBase *defn;
    while ((defn = (ASTBase*)parse_defn_var(ctx)) || (defn = (ASTBase*)parse_defn_fn(ctx))) {
        defn->parent = (ASTBase*)tl;
        ast_list_add(&defns, defn);
    }
    
    parse_end(ctx);
    
    tl->base.location = parsed_source_location(ctx, s);
    tl->definitions = defns;
    
    return tl;
}

void parser_parse(Context *ctx) {
    ASTTopLevel *tl = parse_toplevel(ctx);
    if (!tl) {
        exit(ERR_PARSE);
    }
    
    ctx->ast = tl;
}
