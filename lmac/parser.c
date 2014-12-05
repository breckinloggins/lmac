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

#include <limits.h>
#include <ctype.h>

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
ASTTypeExpression *parse_type_expression(Context *ctx);
ASTTypeConstant *parse_type_constant(Context *ctx);
ASTTypeExpression *parse_type_expression_or_placeholder(Context *ctx);


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
	| '(' type_expression ')' cast_expression
 *
 * NOTE(bloggins): We have to do a little more work because type expressions
 * look like ordinary expressions structurally.
 */
ASTExpression *parse_cast_expression(Context *ctx) {
    ASTExpression *expr = parse_unary_expression(ctx);
    if (expr == NULL) {
        return NULL;
    }

    if (AST_BASE(expr)->kind != AST_EXPR_PAREN) {
        return expr;
    }
    
    ASTExprParen *paren = (ASTExprParen*)expr;
    if (!ast_node_is_type_expression(AST_BASE(paren->inner))) {
        // This is an ordinary non-type-expression
        return expr;
    }
    
    // If we got here we have what looks like a cast. So we should figure out
    // if we have an expression after it. That will tell us for sure
    Context s = snapshot(ctx);
    
    ASTTypeExpression *type_expr = (ASTTypeExpression*)((ASTExprParen*)expr)->inner;
    ASTExpression *next_expr = parse_cast_expression(ctx);
    if (next_expr == NULL) {
        // Looks like it was just a regular parens expression
        return expr;
    }
    
    ASTExprCast *cast_expr = ast_create_expr_cast();
    AST_BASE(type_expr)->parent = (ASTBase*)cast_expr;
    AST_BASE(next_expr)->parent = (ASTBase*)cast_expr;
    AST_BASE(cast_expr)->location = parsed_source_location(ctx, s);
    cast_expr->type = type_expr;
    cast_expr->expr = next_expr;
    
    return (ASTExpression*)cast_expr;
}

/*
 unary_expression
     : postfix_expression
     | TOK_INC unary_expression
     | TOK_DEC unary_expression
     | unary_operator cast_expression
     | TOK_KW_SIZEOF unary_expression
     | TOK_KW_SIZEOF '(' type_name ')'
     | TOK_KW_ALIGNOF '(' type_name ')'
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
    ASTExpression *expr = parse_primary_expression(ctx);
    if (expr == NULL) {
        return NULL;
    }
    
    // TODO(bloggins): This is a hack to prevent casts from looking
    // like function calls. If the expression on the left is a type
    // expression, then don't treat it as indexable or callable or whatever.
    //
    // In the future, I'd like to do the opposite. I want to treat the TYPE
    // as callable, indexible, etc. and implement casts that way. That also
    // allows for overloadable casting in a cool way.
    if (AST_BASE(expr)->kind == AST_EXPR_PAREN) {
        ASTExprParen *paren = (ASTExprParen*)expr;
        if (ast_node_is_type_expression((ASTBase*)paren->inner)) {
            return expr;
        }
    }
    
    for (;;) {
        Token t = peek_token(ctx);
        if (t.kind == TOK_LPAREN) {
            // Assume we have a callable
            next_token(ctx);  // gobble gobble
            
            // TODO: ARGS
            
            expect_token(ctx, TOK_RPAREN);
        } else {
            break;
        }
    }
    
    return expr;
}

/* primary_expression: 
 type_expression
 | TOK_IDENT
 | constant 
 | string
 | '(' type_expression ')' | '(' expression ')' 
 | generic_selection 
 */
ASTExpression *parse_primary_expression(Context *ctx) {
    Context s = snapshot(ctx);
    
    // TODO(bloggins): Break these out
    ASTExpression *expr = (ASTExpression*)parse_type_expression(ctx);
    if (expr != NULL) {
        return expr;
    }
    
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
                // This is last because it's unlikely
                expr = (ASTExpression*)parse_type_expression(ctx);
                if (expr == NULL) {
                    goto fail_parse;
                }
            }
        }
    }
    
    AST_BASE(expr)->location = parsed_source_location(ctx, s);
    return expr;
    
fail_parse:
    restore(ctx, s);
    return NULL;
}


/* ====== Type Expressions ====== */
#pragma mark Type Expressions

ASTTypeExpression *parse_type_expression_or_placeholder(Context *ctx) {
    ASTTypeExpression *expr = parse_type_expression(ctx);
    if (expr != NULL) {
        return expr;
    }
    
    // Well, do we have an ident? Because if so it could be a type name
    // we don't know yet
    ASTIdent *ident = parse_ident(ctx);
    if (ident == NULL) {
        return NULL;
    }
    
    ASTTypeName *type_ph = ast_create_type_name();
    AST_BASE(type_ph)->location = AST_BASE(ident)->location;
    AST_BASE(ident)->parent = (ASTBase*)type_ph;
    
    type_ph->name = ident;
    return (ASTTypeExpression*)type_ph;
}

ASTTypeExpression *parse_type_expression(Context *ctx) {
    return (ASTTypeExpression*)parse_type_constant(ctx);
}

ASTTypeConstant *parse_type_constant(Context *ctx) {
    Context s = snapshot(ctx);
    
    Token tdollar = accept_token(ctx, TOK_DOLLAR);
    if (IS_TOKEN_NONE(tdollar)) { goto fail_parse; }
    
    uint32_t type_id = 0;
    uint8_t bit_flags = BIT_FLAG_NONE;
    uint64_t bit_size = 0;
    
    if (!IS_TOKEN_NONE(accept_token(ctx, TOK_DOLLAR))) {
        
        // In the future we could support higher-kinds and dependent
        // type infrastructure by recursively parsing type expressions,
        // but not now. Only accept the unit type for the meta-kind
        expect_token(ctx, TOK_LPAREN);
        expect_token(ctx, TOK_RPAREN);
        
        type_id |= TYPE_FLAG_SINGLETON;
        type_id |= TYPE_FLAG_KIND;
    } else if (!IS_TOKEN_NONE(accept_token(ctx, TOK_LPAREN))) {
        Token tnum = accept_token(ctx, TOK_NUMBER);
        if (IS_TOKEN_NONE(tnum)) {
            // It's the unit type
            type_id |= TYPE_FLAG_UNIT;
        } else {
            // TODO(bloggins): Implement them!
            diag_printf(DIAG_FATAL, &tnum.location, "singleton types are not yet implemented");
            exit(ERR_PARSE);
        }
        
        expect_token(ctx, TOK_RPAREN);
    } else {
    
        // At this point we are expecting either an IDENT (which should then be of
        // a certain form), or a number
        Token tbit_size = accept_token(ctx, TOK_NUMBER);
        if (IS_TOKEN_NONE(tbit_size)) {
            // It must be embedded in the ident
            Token ttype_desc = expect_token(ctx, TOK_IDENT);
            Spelling desc_sp = ttype_desc.location.spelling;
            const char *desc = spelling_cstring(desc_sp);
            
            switch (desc[0]) {
                case 'f': {
                    bit_flags |= BIT_FLAG_FP;
                    bit_flags |= BIT_FLAG_SIGNED; /* just to be sure */
                } break;
                case 's': {
                    bit_flags |= BIT_FLAG_SIGNED;
                } break;
                case 'u': {
                    bit_flags &= ~(BIT_FLAG_SIGNED);
                } break;
                default: {
                    diag_printf(DIAG_ERROR, &ttype_desc.location, "type constant "
                                "description '%s' is invalid. Unrecognized "
                                " option '%c'", desc, desc[0]);
                    exit(ERR_PARSE);
                } break;
            }
            
            if (!isdigit(desc[1])) {
                diag_printf(DIAG_ERROR, &ttype_desc.location, "type constant "
                            "description '%s' is invalid. Expected bit size"
                            , desc);
                exit(ERR_PARSE);
            }
            
            bit_size = strtoul(desc+1, NULL, 10);
        } else {
            bit_flags |= BIT_FLAG_SIGNED;
            bit_size = strtoul(spelling_cstring(tbit_size.location.spelling), NULL, 10);
        }
        
        type_id |= ast_type_next_type_id();
        
        SourceLocation sl = parsed_source_location(ctx, s);
        // Clean up the source location a bit so it starts right at the dollar sign
        sl.range_start = tdollar.location.range_start;
        
        if (bit_size > WORD_BIT) {
            diag_printf(DIAG_ERROR, &sl, "invalid type size. %d is greater "
                        "than the machine word size of %d bits", bit_size, WORD_BIT);
            exit(ERR_PARSE);
        }
    }
    ASTTypeConstant *type = ast_create_type_constant();
    
    SourceLocation sl = parsed_source_location(ctx, s);
    // Clean up the source location a bit so it starts right at the dollar sign
    sl.range_start = tdollar.location.range_start;

    AST_BASE(type)->location = sl;
    type->base.type_id = type_id;
    type->bit_flags = bit_flags;
    type->bit_size = bit_size;
    
    return type;
    
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
    
    ASTTypeExpression *type = parse_type_expression_or_placeholder(ctx);
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
    AST_BASE(type)->parent = name->base.parent = AST_BASE(expr)->parent = (ASTBase*)defn;
    
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

ASTBase *parse_stmt_expression(Context *ctx) {
    ASTBase *expr = (ASTBase*)parse_expression(ctx);
    if (expr != NULL) {
        expect_token(ctx, TOK_SEMICOLON);
        return expr;
    }
    
    // We could also have an empty expression and just a semi-colon
    Context s = snapshot(ctx);
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_SEMICOLON))) { goto fail_parse; }
    
    return (ASTBase*)ast_create_expr_empty();
    
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
        stmt = (ASTBase*)parse_stmt_expression(ctx);
    }
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
    ASTTypeExpression *type = parse_type_expression_or_placeholder(ctx);
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
    AST_BASE(type)->parent = name->base.parent = (ASTBase*)defn;
    
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
