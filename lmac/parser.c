//
//  parser.c
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// NOTE(bloggins): See http://www.quut.com/c/ANSI-C-grammar-y.html (but this
//                  grammar is left-recursive
//
//                 http://www.ssw.uni-linz.ac.at/Coco/C/C.atg doesn't look
//                 as accurate but appears to be LL

#include "clite.h"

ASTExpression *parse_expression(Context *ctx);
ASTExpression *parse_primary_expression(Context *ctx);
ASTExpression *parse_postfix_expression(Context *ctx);
ASTExpression *parse_unary_expression(Context *ctx);
ASTExpression *parse_cast_expression(Context *ctx);
ASTExpression *parse_multiplicative_expression(Context *ctx);
ASTExpression *parse_additive_expression(Context *ctx);
ASTExpression *parse_shift_expression(Context *ctx);
ASTExpression *parse_relational_expression(Context *ctx);
ASTExpression *parse_equality_expression(Context *ctx);
ASTExpression *parse_and_expression(Context *ctx);
ASTExpression *parse_exclusive_or_expression(Context *ctx);
ASTExpression *parse_inclusve_or_expression(Context *ctx);
ASTExpression *parse_logical_and_expression(Context *ctx);
ASTExpression *parse_logical_or_expression(Context *ctx);
ASTExpression *parse_conditional_expression(Context *ctx);
ASTExpression *parse_assignment_expression(Context *ctx);
ASTPPDirective *parse_pp_directive(Context *ctx);

#pragma mark Parse Utility Functions

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

//
// Parse routines
//
/* ====== Identifiers ====== */
#pragma mark Identifiers

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


/* ====== Expressions ====== */
#pragma mark Expressions

/*
 expression
	: assignment_expression
	| expression ',' assignment_expression
 */
ASTExpression *parse_expression(Context *ctx) {
    return parse_assignment_expression(ctx);
}

/*
 assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
 */
ASTExpression *parse_assignment_expression(Context *ctx) {
    return parse_conditional_expression(ctx);
}

/*
 conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
 */
ASTExpression *parse_conditional_expression(Context *ctx) {
    return parse_logical_or_expression(ctx);
}

/*
 logical_or_expression
	: logical_and_expression
	| logical_or_expression TOK_OROR logical_and_expression
 */
ASTExpression *parse_logical_or_expression(Context *ctx) {
    return parse_logical_and_expression(ctx);
}

/*
 logical_and_expression
	: inclusive_or_expression
	| logical_and_expression TOK_ANDAND inclusive_or_expression
 */
ASTExpression *parse_logical_and_expression(Context *ctx) {
    return parse_inclusve_or_expression(ctx);
}

/*
 inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
 */
ASTExpression *parse_inclusve_or_expression(Context *ctx) {
    return parse_exclusive_or_expression(ctx);
}

/*
 exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
 */
ASTExpression *parse_exclusive_or_expression(Context *ctx) {
    return parse_and_expression(ctx);
}

/*
 and_expression
	: equality_expression
	| and_expression '&' equality_expression
 */
ASTExpression *parse_and_expression(Context *ctx) {
    return parse_equality_expression(ctx);
}

/*
 equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
 */
ASTExpression *parse_equality_expression(Context *ctx) {
    return parse_relational_expression(ctx);
}

/*
 relational_expression
	: shift_expression
	| relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression TOK_LTE shift_expression
	| relational_expression TOK_GTE shift_expression
 */
ASTExpression *parse_relational_expression(Context *ctx) {
    return parse_shift_expression(ctx);
}

/*
 shift_expression
	: additive_expression
	| shift_expression TOK_LEFT additive_expression
	| shift_expression TOK_RIGHT additive_expression
 */
ASTExpression *parse_shift_expression(Context *ctx) {
    return parse_additive_expression(ctx);
}

/*
 additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
 */
ASTExpression *parse_additive_expression(Context *ctx) {
    ASTExpression *expr = parse_multiplicative_expression(ctx);
    
    for (;;) {
        Token t = peek_token(ctx);
        if (t.kind == TOK_PLUS || t.kind == TOK_MINUS) {
            char op = (char)*t.location.range_start;
            
            // Assume we have a binary expression
            next_token(ctx);  // gobble gobble
            
            ASTExpression *left = expr;
            ASTExpression *right = parse_multiplicative_expression(ctx);
            if (right == NULL) {
                SourceLocation sl = parsed_source_location(ctx, *ctx);
                diag_printf(DIAG_ERROR, &sl, "expected expression after '%c'", op);
                exit(ERR_PARSE);
            }
            
            // TODO(bloggins): move to ast.c in function like "ast_init_expr_binary(...)"
            ASTExprBinary *binop = ast_create_expr_binary();
            ASTOperator *op_node = ast_create_operator();
            op_node->base.location = t.location;
            op_node->op = op;
            ast_init_expr_binary(binop, left, right, op_node);
            
            expr = (ASTExpression*)binop;
        } else {
            break;
        }
    }
    
    return expr;
}

/*
 multiplicative_expression
	: cast_expression
	| multiplicative_expression '*' cast_expression
	| multiplicative_expression '/' cast_expression
	| multiplicative_expression '%' cast_expression
 */
ASTExpression *parse_multiplicative_expression(Context *ctx) {
    ASTExpression *expr = parse_cast_expression(ctx);

    for (;;) {
        Token t = peek_token(ctx);
        if (t.kind == TOK_STAR || t.kind == TOK_FORWARDSLASH || t.kind == TOK_PERCENT) {
            char op = (char)*t.location.range_start;
            
            // Assume we have a binary expression
            next_token(ctx);  // gobble gobble
            
            ASTExpression *left = expr;
            ASTExpression *right = parse_cast_expression(ctx);
            if (right == NULL) {
                SourceLocation sl = parsed_source_location(ctx, *ctx);
                diag_printf(DIAG_ERROR, &sl, "expected expression after '%c'", op);
                exit(ERR_PARSE);
            }
            
            // TODO(bloggins): move to ast.c in function like "ast_init_expr_binary(...)"
            ASTExprBinary *binop = ast_create_expr_binary();
            ASTOperator *op_node = ast_create_operator();
            op_node->base.location = t.location;
            op_node->op = op;
            ast_init_expr_binary(binop, left, right, op_node);
            
            expr = (ASTExpression*)binop;
        } else {
            break;
        }
    }
    
    return expr;
}

/*
 cast_expression
	: unary_expression
	| '(' type_name ')' cast_expression
 */
ASTExpression *parse_cast_expression(Context *ctx) {
    return parse_unary_expression(ctx);
}

/*
 unary_expression
     : postfix_expression
     | TOK_INC unary_expression
     | TOK_DEC unary_expression
     | unary_operator cast_expression
     | TOK_SIZEOF unary_expression
     | TOK_SIZEOF '(' type_name ')'
     | TOK_ALIGNOF '(' type_name ')'
 */
ASTExpression *parse_unary_expression(Context *ctx) {
    return parse_postfix_expression(ctx);
}

/*
 postfix_expression
     :primary_expression
     | postfix_expression '[' expression ']'
     | postfix_expression '(' ')'
     | postfix_expression '(' argument_expression_list ')'
     | postfix_expression '.' TOK_IDENT
     | postfix_expression TOK_PTR TOK_IDENT
     | postfix_expression TOK_INC
     | postfix_expression TOK_DEC
     | '(' type_name ')' '{' initializer_list '}'
     | '(' type_name ')' '{' initializer_list ',' '}'
 */
ASTExpression *parse_postfix_expression(Context *ctx) {
    return parse_primary_expression(ctx);
}

/* primary_expression: TOK_IDENT | constant | string | '(' expression ')' | generic_selection */
ASTExpression *parse_primary_expression(Context *ctx) {
    Context s = snapshot(ctx);
    
    // TODO(bloggins): Break these out
    ASTExpression *expr = NULL;
    Token t = accept_token(ctx, TOK_IDENT);
    if (!IS_TOKEN_NONE(t)) {
        ASTExprIdent *ident = ast_create_expr_ident();
        ASTIdent *name = ast_create_ident();
        name->base.location = t.location;
        name->base.parent = (ASTBase*)ident;
        ident->name = name;
        
        expr = (ASTExpression*)ident;
    } else {
        t = accept_token(ctx, TOK_NUMBER);
        if (!IS_TOKEN_NONE(t)) {
            ASTExprNumber *number = ast_create_expr_number();

            AST_BASE(number)->location = t.location;
            number->number = (int)strtol((char*)t.location.range_start, NULL, 10);
            
            expr = (ASTExpression*)number;
        } else {
            t = accept_token(ctx, TOK_LPAREN);
            if (!IS_TOKEN_NONE(t)) {
                ASTExpression *inner = parse_expression(ctx);
                t = expect_token(ctx, TOK_RPAREN);
                if (inner == NULL) {
                    diag_printf(DIAG_ERROR, &t.location, "expected expression");
                    exit(ERR_PARSE);
                }
                
                ASTExprParen *paren = ast_create_expr_paren();
                AST_BASE(paren)->location = t.location;
                AST_BASE(inner)->parent = (ASTBase*)paren;
                
                paren->inner = inner;
                expr = (ASTExpression*)paren;
                
            } else {
                goto fail_parse;
            }
        }
    }
    
    AST_BASE(expr)->location = parsed_source_location(ctx, s);
    return expr;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}


/* ====== Misc (Needs categorization) ====== */
#pragma mark Misc

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
    
    ASTExpression *expr = parse_expression(ctx);
    if (expr == NULL) { goto fail_parse; }
    
    t = expect_token(ctx, TOK_SEMICOLON);
    
    ASTDefnVar *defn = ast_create_defn_var();
    defn->base.location = parsed_source_location(ctx, s);
    type->base.parent = name->base.parent = AST_BASE(expr)->parent = (ASTBase*)defn;
    
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
    
    ASTExpression *expr = parse_expression(ctx);
    if (expr == NULL) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "expected expression");
        exit(ERR_PARSE);
    }
    
    expect_token(ctx, TOK_SEMICOLON);
    
    ASTStmtReturn *stmt = ast_create_stmt_return();
    stmt->base.location = parsed_source_location(ctx, s);
    AST_BASE(expr)->parent = (ASTBase *)stmt;
    
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
    ASTList *stmts = NULL;
    
    ASTTopLevel *tl = ast_create_toplevel();
    ASTBase *stmt;
    while ((stmt = (ASTBase*)parse_defn_var(ctx)) ||
           (stmt = (ASTBase*)parse_defn_fn(ctx)) ||
           (stmt = (ASTBase*)parse_pp_directive(ctx))) {
        stmt->parent = (ASTBase*)tl;
        ast_list_add(&stmts, stmt);
    }
    
    parse_end(ctx);
    
    tl->base.location = parsed_source_location(ctx, s);
    tl->definitions = stmts;
    
    return tl;
}


/* ====== Preprocessor ====== */
#pragma mark Preprocessor

ASTPPDirective *parse_pp_directive(Context *ctx) {
    // NOTE(bloggins): Eventually the preprocessor will do something fancy, but
    // for right now preprocessor tokens will just be inserted into the AST
    Context s = snapshot(ctx);
    
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_HASH))) { goto fail_parse; }
    
    ASTIdent *directive = parse_ident(ctx);
    if (directive == NULL) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "invalid syntax following preprocessor directive");
        exit(ERR_PARSE);
    }
    
    if (!spelling_streq(directive->base.location.spelling, "pragma")) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "unrecognized preprocessor directive '%s'",
                    spelling_cstring(directive->base.location.spelling));
        exit(ERR_PARSE);
    }
    
    // This is a hack to make sure we only parse pragma directives on the line
    // the pragma was defined on.
    uint32_t lineof_directive = directive->base.location.line;
    
    // TODO(bloggins): This is not valid C syntax for pragma. We do this for
    // now until we have a more complete pre-processor.
    ASTIdent *arg1 = parse_ident(ctx);
    if (arg1 == NULL || arg1->base.location.line != lineof_directive) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "argument expected after pragma");
        exit(ERR_PARSE);
    }
    
    if (!spelling_streq(arg1->base.location.spelling, "CLITE")) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "unsupported argument '%s' after pragma. Only 'CLITE' is supported",
                    spelling_cstring(arg1->base.location.spelling));
        exit(ERR_PARSE);
        
        
    }

    // This is the actual argument to be used for directives
    ASTIdent *arg2 = parse_ident(ctx);
    if (arg2 == NULL || arg2->base.location.line != lineof_directive) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_printf(DIAG_ERROR, &sl, "second argument expected after pragma");
        exit(ERR_PARSE);
    }
    
    ASTPPPragma *pragma = ast_create_pp_pragma();
    AST_BASE(pragma)->location = parsed_source_location(ctx, s);
    AST_BASE(arg2)->parent = (ASTBase*)pragma;
    pragma->arg = arg2;
    return (ASTPPDirective*)pragma;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}

/* ====== Public API ====== */
#pragma mark Public API

void parser_parse(Context *ctx) {
    ASTTopLevel *tl = parse_toplevel(ctx);
    if (!tl) {
        exit(ERR_PARSE);
    }
    
    ctx->ast = tl;
}
