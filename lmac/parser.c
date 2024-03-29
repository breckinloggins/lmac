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
#include <stdarg.h>

bool parse_declaration(Context *ctx, ASTDeclaration **result);

bool parse_block_item(Context *ctx, ASTBase **result);

bool parse_statement(Context *ctx, ASTBase **result);
bool parse_stmt_compound(Context *ctx, ASTBlock **result);

bool parse_expression(Context *ctx, ASTExpression **result);
bool parse_expr_primary(Context *ctx, ASTExpression **result);
bool parse_expr_postfix(Context *ctx, ASTExpression **result);
bool parse_expr_unary(Context *ctx, ASTExpression **result);
bool parse_expr_cast(Context *ctx, ASTExpression **result);
bool parse_expr_multiplicative(Context *ctx, ASTExpression **result);
bool parse_expr_additive(Context *ctx, ASTExpression **result);
bool parse_expr_shift(Context *ctx, ASTExpression **result);
bool parse_expr_relational(Context *ctx, ASTExpression **result);
bool parse_expr_equality(Context *ctx, ASTExpression **result);
bool parse_expr_and(Context *ctx, ASTExpression **result);
bool parse_expr_exclusive_or(Context *ctx, ASTExpression **result);
bool parse_expr_inclusive_or(Context *ctx, ASTExpression **result);
bool parse_expr_logical_and(Context *ctx, ASTExpression **result);
bool parse_expr_logical_or(Context *ctx, ASTExpression **result);
bool parse_expr_conditional(Context *ctx, ASTExpression **result);
bool parse_expr_assignment(Context *ctx, ASTExpression **result);

bool parse_pp_directive(Context *ctx, ASTPPDirective **result);

bool parse_type_expression(Context *ctx, ASTTypeExpression **result);
bool parse_type_constant(Context *ctx, ASTTypeConstant **result);
bool parse_type_name(Context *ctx, ASTTypeName **result);


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
    SourceLocation sl = {0};
    sl.file = ctx->file;
    sl.line = snapshot.line;
    sl.ctx = ctx;
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
            diag_emit(DIAG_FATAL, ERR_LEX, NULL, "unknown token '%s'", spelling_cstring(t.location.spelling));
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
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "expected %s", token_get_name(kind));
    }
    
    return t;
}

void ap_expect_node_out(Context *ctx, ParseFn parser, ASTBase **result,
                        const char *msg_fmt, va_list args) {
    assert(ctx);
    assert(parser);
    if (!parser(ctx, result)) {
        SourceLocation sl = parsed_source_location(ctx, *ctx);
        
        diag_vfemit(DIAG_ERROR, ERR_PARSE, &sl, stderr, msg_fmt, args);
    }
    
    /* TODO(bloggins): Subkind checking */
    /*
     if (node != NULL) {
     assert(node->kind == kind);
     }
     */
    
}

void expect_node_out(Context *ctx, ParseFn parser, ASTBase **result,
                 const char *msg_fmt, ...) {
    va_list ap;
    va_start(ap, msg_fmt);
    ap_expect_node_out(ctx, parser, result, msg_fmt, ap);
    va_end(ap);

}

ASTBase *expect_node(Context *ctx, ParseFn parser, const char *msg_fmt, ...) {
    ASTBase *node = NULL;
    
    va_list ap;
    va_start(ap, msg_fmt);
    ap_expect_node_out(ctx, parser, &node, msg_fmt, ap);
    va_end(ap);

    return node;
}

//
// Parse routines
//
/* ====== Identifiers ====== */
#pragma mark Identifiers

bool parse_ident(Context *ctx, ASTIdent **result) {
    Context s = snapshot(ctx);
    
    Token t = accept_token(ctx, TOK_IDENT);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    assert(t.location.ctx);
    act_on_ident(t.location, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}


/* ====== Expressions ====== */
#pragma mark Expressions

bool parse_next_expr_binary(Context *ctx, TokenKind allowed_ops[],
    bool (*parse_expr_right)(Context *ctx, ASTExpression **right),
                            ASTExpression **result) {
    assert(result && "must pass a valid result pointer");
    
    Token t = peek_token(ctx);
    if (!token_is_of_kind(t, allowed_ops)) {
        return false;
    }
    
    // Assume we have a binary expression
    next_token(ctx);  // gobble gobble
    
    ASTExpression *left = *result;
    ASTExpression *right = (ASTExpression*)expect_node(ctx, (ParseFn)parse_expr_right,
                                       "expected expression");
    
    act_on_expr_binary(t.location, left, right, t, (ASTExprBinary**)result);
    return true;
}

/*
 expression
	: assignment_expression
	| expression ',' assignment_expression
 */
bool parse_expression(Context *ctx, ASTExpression **result) {
    return parse_expr_assignment(ctx, result);
}

/*
 assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
 */
bool parse_expr_assignment(Context *ctx, ASTExpression **result) {
    Context s = snapshot(ctx);
    
    // TODO(bloggins): This isn't exactly right, but parse_expr_conditional
    // includes parsing a unitary expression so what we should really be doing
    // is checking for something like is_unary and only proceeding further below
    // if that's indeed the case. Otherwise, we're going to have to do the checking
    // farther up in the semantic analyzer and test whether what we just parsed
    // is a valid lvalue
    ASTExpression *left = NULL;
    if (!parse_expr_conditional(ctx, &left)) { return false; }
    
    Token t = accept_token(ctx, TOK_EQUALS);
    if (IS_TOKEN_NONE(t)) {
        if (result != NULL) {
            *result = left;
        }
        return true;
    }
    
    // We have an equals so we must be an assignment expression. Keep going
    ASTExpression *right = (ASTExpression*)expect_node(ctx, (ParseFn)parse_expr_assignment, "expected expression after assignment");
    act_on_expr_binary(parsed_source_location(ctx, s), left, right, t, (ASTExprBinary**)result);
    
    return true;
    
}

/*
 conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
 */
bool parse_expr_conditional(Context *ctx, ASTExpression **result) {
    return parse_expr_logical_or(ctx, result);
}

/*
 logical_or_expression
	: logical_and_expression
	| logical_or_expression TOK_PIPE_PIPE logical_and_expression
 */
bool parse_expr_logical_or(Context *ctx, ASTExpression **result) {
    if (!parse_expr_logical_and(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_PIPE_PIPE, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_logical_and, result)) {
        // Keep going
    }
    
    return true;
}

/*
 logical_and_expression
	: inclusive_or_expression
	| logical_and_expression TOK_AMP_AMP inclusive_or_expression
 */
bool parse_expr_logical_and(Context *ctx, ASTExpression **result) {
    if (!parse_expr_inclusive_or(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_AMP_AMP, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_inclusive_or, result)) {
        // Keep going
    }
    
    return true;
}

/*
 inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression TOK_PIPE exclusive_or_expression
 */
bool parse_expr_inclusive_or(Context *ctx, ASTExpression **result) {
    if (!parse_expr_exclusive_or(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_PIPE, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_exclusive_or, result)) {
        // Keep going
    }
    
    return true;
}

/*
 exclusive_or_expression
	: and_expression
	| exclusive_or_expression TOK_CARET and_expression
 */
bool parse_expr_exclusive_or(Context *ctx, ASTExpression **result) {
    if (!parse_expr_and(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_CARET, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_and, result)) {
        // Keep going
    }
    
    return true;
}

/*
 and_expression
	: equality_expression
	| and_expression TOK_AMP equality_expression
 */
bool parse_expr_and(Context *ctx, ASTExpression **result) {
    if (!parse_expr_equality(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_AMP, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_equality, result)) {
        // Keep going
    }
    
    return true;
}

/*
 equality_expression
	: relational_expression
	| equality_expression TOK_EQUALS_EQUALS relational_expression
	| equality_expression TOK_BANG_EQUALS relational_expression
 */
bool parse_expr_equality(Context *ctx, ASTExpression **result) {
    if (!parse_expr_relational(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_EQUALS_EQUALS, TOK_BANG_EQUALS, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_relational, result)) {
        // Keep going
    }
    
    return true;
}

/*
 relational_expression
	: shift_expression
	| relational_expression TOK_LANGLE shift_expression
	| relational_expression TOK_RANGLE shift_expression
	| relational_expression TOK_LANGLE_EQUALS shift_expression
	| relational_expression TOK_RANGLE_EQUALS shift_expression
 */
bool parse_expr_relational(Context *ctx, ASTExpression **result) {
    if (!parse_expr_shift(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_LANGLE, TOK_RANGLE, TOK_LANGLE_EQUALS,
        TOK_RANGLE_EQUALS, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_shift, result)) {
        // Keep going
    }
    
    return true;
}

/*
 shift_expression
	: additive_expression
	| shift_expression TOK_2LANGLE additive_expression
	| shift_expression TOK_2RANGLE additive_expression
 */
bool parse_expr_shift(Context *ctx, ASTExpression **result) {
    if (!parse_expr_additive(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_2LANGLE, TOK_2RANGLE, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_additive, result)) {
        // Keep going
    }
    
    return true;
}

/*
 additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
 */
bool parse_expr_additive(Context *ctx, ASTExpression **result) {
    // TODO(bloggins): extract this into parse_expr_binary(ctx, [valid_operator_kinds], result)
    if (!parse_expr_multiplicative(ctx, result)) {
        return false;
    }
    
    TokenKind ops[] = {TOK_PLUS, TOK_MINUS, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_multiplicative, result)) {
        // Keep going
    }
    
    return true;
}

/*
 multiplicative_expression
	: cast_expression
	| multiplicative_expression '*' cast_expression
	| multiplicative_expression '/' cast_expression
	| multiplicative_expression '%' cast_expression
 */
bool parse_expr_multiplicative(Context *ctx, ASTExpression **result) {
    if (!parse_expr_cast(ctx, result)) {
        return NULL;
    }
    
    TokenKind ops[] = {TOK_STAR, TOK_FORWARDSLASH, TOK_PERCENT, TOK_LAST};
    while (parse_next_expr_binary(ctx, ops, parse_expr_cast, result)) {
        // Keep going
    }
    
    return true;
}

/*
 cast_expression
	: unary_expression
	| '(' type_expression ')' cast_expression
 *
 * NOTE(bloggins): We have to do a little more work because type expressions
 * look like ordinary expressions structurally.
 */
bool parse_expr_cast(Context *ctx, ASTExpression **result) {
    if (!parse_expr_unary(ctx, result)) {
        return NULL;
    }

    // We can't make any parse decisions without the actual result.
    // (note that this shows a dependency on parse context that we probably
    // want to fix if we can)
    if (result == NULL) {
        SourceLocation sl = parsed_source_location(ctx, *ctx);
        diag_emit(DIAG_FATAL, ERR_PARSE, &sl,
                    "parse_expr_cast currently cannot parse correctly "
                    "without AST construction");
    }
    
    if (!AST_IS(*result, AST_EXPR_PAREN)) {
        return true;
    }
    
    ASTExprParen *paren = (ASTExprParen*)*result;
    if (!ast_node_is_type_expression(AST_BASE(paren->inner))) {
        // This is an ordinary non-type-expression
        return true;
    }
    
    // If we got here we have what looks like a cast. So we should figure out
    // if we have an expression after it. That will tell us for sure
    Context s = snapshot(ctx);
    
    ASTTypeExpression *type_expr = (ASTTypeExpression*)((ASTExprParen*)*result)->inner;
    ASTExpression *next_expr = NULL;
    if (!parse_expr_cast(ctx, &next_expr)) {
        // Looks like it was just a regular parens expression
        return true;
    }
    
    act_on_expr_cast(parsed_source_location(ctx, s), type_expr, next_expr,
                     (ASTExprCast**)result);
    return true;
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
bool parse_expr_unary(Context *ctx, ASTExpression **result) {
    return parse_expr_postfix(ctx, result);
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
bool parse_expr_postfix(Context *ctx, ASTExpression **result) {
    if (!parse_expr_primary(ctx, result)) {
        return false;
    }
    
    // TODO(bloggins): This is a hack to prevent casts from looking
    // like function calls. If the expression on the left is a type
    // expression, then don't treat it as indexable or callable or whatever.
    //
    // In the future, I'd like to do the opposite. I want to treat the TYPE
    // as callable, indexible, etc. and implement casts that way. That also
    // allows for overloadable casting in a cool way.
    if (*result && AST_IS(*result, AST_EXPR_PAREN)) {
        ASTExprParen *paren = (ASTExprParen*)*result;
        if (ast_node_is_type_expression((ASTBase*)paren->inner)) {
            return true;
        }
    }
    
    for (;;) {
        Token t = peek_token(ctx);
        if (t.kind == TOK_LPAREN) {
            // Assume we have a callable
            next_token(ctx);  // gobble gobble
            
            List *args = NULL;
            for (;;) {
                ASTExpression *expr = NULL;
                if (!parse_expression(ctx, &expr)) {
                    break;
                }
                
                list_append(&args, expr);
                
                if (IS_TOKEN_NONE(accept_token(ctx, TOK_COMMA))) {
                    break;
                }
            }
            
            expect_token(ctx, TOK_RPAREN);
            
            if (result != NULL) {
                ASTExpression *callable = *result;
                act_on_expr_call(t.location, callable, args, (ASTExprCall**)result);
            }
        } else {
            break;
        }
    }
    
    return true;
}

bool parse_expr_string(Context *ctx, ASTExprString **result) {
    Context s = snapshot(ctx);
    
    // TODO(bloggins): Handle escapes
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_DOUBLE_QUOTE))) { goto fail_parse; }
    
    uint8_t *range_start = ctx->pos;
    while ((char)*(ctx->pos++) != '"') {
        if ((char)*(ctx->pos) == '\n') {
            SourceLocation sl = parsed_source_location(ctx, s);
            diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "unterminated string constant");
        }
    }
    uint8_t *range_end = ctx->pos;
    
    SourceLocation sl = parsed_source_location(ctx, s);
    sl.range_start = range_start;
    sl.range_end = range_end - 1;
    act_on_expr_string(sl, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

/* primary_expression: 
 type_expression
 | TOK_IDENT
 | constant 
 | string
 | '(' type_expression ')' | '(' expression ')' 
 | generic_selection
 | preprocessor expression
 */
bool parse_expr_primary(Context *ctx, ASTExpression **result) {
    Context s = snapshot(ctx);
    
    // TODO(bloggins): Break these out
    if(parse_type_expression(ctx, (ASTTypeExpression**)result)) { return true; }
    
    Token t = accept_token(ctx, TOK_IDENT);
    if (!IS_TOKEN_NONE(t)) {
        assert(t.location.ctx);
        act_on_expr_ident(t.location, (ASTExprIdent**)result);
    } else if (!parse_expr_string(ctx, (ASTExprString**)result)) {
        t = accept_token(ctx, TOK_NUMBER);
        if (!IS_TOKEN_NONE(t)) {
            int n = (int)strtol((char*)t.location.range_start, NULL, 10);
            act_on_expr_number(t.location, n, (ASTExprNumber**)result);
        } else {
            t = accept_token(ctx, TOK_LPAREN);
            if (!IS_TOKEN_NONE(t)) {
                ASTExpression *inner = (ASTExpression*)expect_node(ctx, (ParseFn)parse_expression, "expected expression");
                t = expect_token(ctx, TOK_RPAREN);
               
                act_on_expr_paren(t.location, inner, (ASTExprParen**)result);
            } else {
                if (!parse_type_expression(ctx, (ASTTypeExpression**)result)) {
                    // This is last because it's unlikely
                    if (parse_pp_directive(ctx, (ASTPPDirective**)result)) {
                        if (result != NULL) {
                            if (!ast_node_is_expression((ASTBase*)*result)) {
                                diag_emit(DIAG_ERROR, ERR_PARSE, &((ASTBase*)*result)->location,
                                            "expected expression from preprocessor");
                            }
                        }
                    } else {
                        goto fail_parse;
                    }
                }
            }
        }
    }
    
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}


/* ====== Type Expressions ====== */
#pragma mark Type Expressions
// TODO(bloggins): See if we can completely unify type expressions with
// normal expressions so we can get rid of everything except the type
// literals. We could do this by having the parse allow arbitrary expressions
// on types and the analyzer would check validity. Similarly, places that
// currently take type expressions would just take epressions and the analyzer
// would determine if the expression is appropriate (for example, if the
// expression is an identifier or pure function call that the compiler can verify
// results in a type, then it would be allowed). For compile-time functions
// that are guaranteed to terminate, look into "Total Functional Programming".

bool parse_type_expression(Context *ctx, ASTTypeExpression **result) {
    if (!parse_type_constant(ctx, (ASTTypeConstant**)result) &&
        !parse_type_name(ctx, (ASTTypeName**)result)) {
        return false;
    }
    
    // Check for pointers
    for (;;) {
        Token t = peek_token(ctx);
        if (t.kind == TOK_STAR) {
            next_token(ctx);  // gobble gobble
            
            // TODO: ARGS
            if (result != NULL) {
                ASTTypeExpression *pointed_to = *result;
                act_on_type_pointer(t.location, pointed_to, (ASTTypePointer**)result);
            }
        } else {
            break;
        }
    }
    
    return true;
}

bool parse_type_name(Context *ctx, ASTTypeName **result) {
    Context s = snapshot(ctx);
    
    ASTIdent *ident = NULL;
    if (!parse_ident(ctx, &ident)) { goto fail_parse; }
    
    if (!ast_ident_is_type_name(ident)) { goto fail_parse; }
    
    act_on_type_name(AST_BASE(ident)->location, ident, (ASTTypeName**)result);
    return true;

fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_type_constant(Context *ctx, ASTTypeConstant **result) {
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
            diag_emit(DIAG_FATAL, ERR_PARSE, &tnum.location, "singleton types are not yet implemented");
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
                case 'c': {
                    bit_flags |= BIT_FLAG_CHAR;
                    bit_flags |= BIT_FLAG_SIGNED;
                } break;
                case 'i': {
                    bit_flags |= BIT_FLAG_INT;
                    bit_flags |= BIT_FLAG_SIGNED;
                } break;
                case 's': {
                    bit_flags |= BIT_FLAG_SIGNED;
                } break;
                case 'u': {
                    bit_flags &= ~(BIT_FLAG_SIGNED);
                } break;
                default: {
                    diag_emit(DIAG_ERROR, ERR_PARSE, &ttype_desc.location, "type constant "
                                "description '%s' is invalid. Unrecognized "
                                " option '%c'", desc, desc[0]);
                } break;
            }
            
            if (!isdigit(desc[1])) {
                diag_emit(DIAG_ERROR, ERR_PARSE, &ttype_desc.location, "type constant "
                            "description '%s' is invalid. Expected bit size"
                            , desc);
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
            diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "invalid type size. %d is greater "
                        "than the machine word size of %d bits", bit_size, WORD_BIT);
        }
    }
    
    SourceLocation sl = parsed_source_location(ctx, s);
    // Clean up the source location a bit so it starts right at the dollar sign
    sl.range_start = tdollar.location.range_start;

    act_on_type_constant(sl, type_id, bit_flags, bit_size, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

/* ====== Declarations ====== */
#pragma mark Declarations

bool parse_decl_var(Context *ctx, ASTDeclVar **result) {
    // TODO(bloggins): Snapshotting works but can be slow (because we might
    //                  backtrack a long way). Should we left-factor instead?
    Context s = snapshot(ctx);
    
    Token t = accept_token(ctx, TOK_KW_CONST);
    bool is_const = !IS_TOKEN_NONE(t);
    
    ASTTypeExpression *type = NULL;
    if (!parse_type_expression(ctx, &type)) {
        if (is_const) {
            diag_emit(DIAG_ERROR, ERR_PARSE, &t.location,
                        "expected type name after '%s'",
                        spelling_cstring(t.location.spelling));
        } else {
            goto fail_parse;
        }
    }
    
    ASTIdent *name = NULL;
    if (!parse_ident(ctx, &name)) {
        if (is_const) {
            diag_emit(DIAG_ERROR, ERR_PARSE, &t.location,
                        "expected variable name after '%s'",
                        spelling_cstring(t.location.spelling));
        } else {
            goto fail_parse;
        }
    }
    
    ASTExpression *expr = NULL;
    t = accept_token(ctx, TOK_EQUALS);
    if (!IS_TOKEN_NONE(t)) {
        expect_node_out(ctx, (ParseFn)parse_expression, (ASTBase**)&expr,
                        "expected expression after assignment");
    }
    
    act_on_decl_var(parsed_source_location(ctx, s), ctx->active_scope,
                    type, is_const, name, expr, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_decl_fn(Context *ctx, ASTDeclFunc **result) {
    Context s = snapshot(ctx);
    
    // TODO(bloggins): Factor this grammar into reusable chunks like
    //                  "parse_begin_decl"
    ASTTypeExpression *type = NULL;
    if (!parse_type_expression(ctx, &type)) { goto fail_parse; }
    
    ASTIdent *name = NULL;
    if (!parse_ident(ctx, &name)) { goto fail_parse; }
    
    Token t = accept_token(ctx, TOK_LPAREN);
    if (IS_TOKEN_NONE(t)) { goto fail_parse; }
    
    // NOTE(bloggins): Params and optional definition will go in their own
    // sub-scope
    context_scope_push(ctx);
    
    bool varargs_found = false;
    List *params = NULL;
    for (;;) {
        // TODO(bloggins): Should accept nameless prototype as well
        ASTDeclVar *decl = NULL;
        if (!parse_decl_var(ctx, &decl)) {
            if (!IS_TOKEN_NONE(accept_token(ctx, TOK_ELLIPSIS))) {
                varargs_found = true;
                // TODO: need to act on varargs decl and add AST node
                break;
            } else {
                break;
            }
        }
        
        list_append(&params, decl);
        
        if (IS_TOKEN_NONE(accept_token(ctx, TOK_COMMA))) {
            break;
        }
        
        if (varargs_found) {
            SourceLocation sl = parsed_source_location(ctx, s);
            diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "variable argument parameter "
                        "must be the last parameter in a function");
        }
    }
    
    expect_token(ctx, TOK_RPAREN);
    
    /* Optional block to define the function */
    ASTBlock *block = NULL;
    if (!parse_stmt_compound(ctx, &block)) {
        expect_token(ctx, TOK_SEMICOLON);
    } else {
        if (varargs_found) {
            SourceLocation sl = parsed_source_location(ctx, s);
            diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "functions with variable arguments "
                        "can be declared but can not currently be defined");
        }
    }
    
    context_scope_pop(ctx);
    act_on_decl_fn(parsed_source_location(ctx, s), ctx->active_scope,
                   type, name, params, varargs_found, block, result);
    
    return true;
    
fail_parse:
    context_scope_pop(ctx);
    restore(ctx, s);
    return false;
}

bool parse_decl_external(Context *ctx, ASTDeclaration **result) {
    // TODO(bloggins): We don't need to check for semicolon here when
    // we have the full parse_declaration grammar
    if (parse_decl_fn(ctx, (ASTDeclFunc**)result)) {
        return true;
    }
    
    ASTDeclaration *decl = NULL;
    if (parse_declaration(ctx, &decl)) {
        expect_token(ctx, TOK_SEMICOLON);
        act_on_stmt_declaration(decl->base.location, decl, (ASTStmtDecl**)result);
        return true;
    }
    
    return false;
}

bool parse_declaration(Context *ctx, ASTDeclaration **result) {
    return parse_decl_var(ctx, (ASTDeclVar**)result);
}

/* ====== Statments ====== */
#pragma mark Statements

bool parse_stmt_return(Context *ctx, ASTStmtReturn **result) {
    Context s = snapshot(ctx);
    
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_KW_RETURN))) { goto fail_parse; }
    
    ASTExpression *expr = (ASTExpression*)expect_node(ctx, (ParseFn)parse_expression, "expected expression");
    expect_token(ctx, TOK_SEMICOLON);
    
    act_on_stmt_return(parsed_source_location(ctx, s), expr, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

// TODO(bloggins): This is not correct. Declarations are not statements.
// It's fixed except for needing to implement the rest of the decl grammar
// so that we don't reuse parse_declaration for parse_decl_fn params.
bool parse_stmt_declaration(Context *ctx, ASTStmtDecl **result) {
    Context s = snapshot(ctx);
    
    ASTDeclaration *decl = NULL;
    if (!parse_declaration(ctx, &decl)) {
        return false;
    }
    expect_token(ctx, TOK_SEMICOLON);
    
    act_on_stmt_declaration(parsed_source_location(ctx, s), decl, result);
    return true;
}

bool parse_stmt_expression(Context *ctx, ASTStmtExpr **result) {
    ASTExpression *expr = NULL;
    Token t = accept_token(ctx, TOK_SEMICOLON);
    if (!IS_TOKEN_NONE(t)) {
        act_on_stmt_expression(t.location, NULL, result);
        return true;
    }
    
    if (!parse_expression(ctx, &expr)) {
        return false;
    }
    
    Context s = snapshot(ctx);
    
    if (expr != NULL) {
        expect_token(ctx, TOK_SEMICOLON);
    } else {
    
        // We could also have an empty expression and just a semi-colon
        s = snapshot(ctx);
        if (IS_TOKEN_NONE(accept_token(ctx, TOK_SEMICOLON))) { goto fail_parse; }
    }
    
    act_on_stmt_expression(parsed_source_location(ctx, s), expr, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_stmt_if(Context *ctx, ASTStmtIf **result) {
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_KW_IF))) {
        return false;
    }
    
    Context s = snapshot(ctx);
    context_scope_push(ctx);
    
    expect_token(ctx, TOK_LPAREN);
    ASTExpression *condition = (ASTExpression*)expect_node(ctx, (ParseFn)parse_expression,
                                                           "expected condition expression");
    expect_token(ctx, TOK_RPAREN);
    
    ASTBase *stmt_true = NULL;
    ASTBase *stmt_false = NULL;
    if (parse_statement(ctx, &stmt_true)) {
        // TODO(bloggins): Solve dangling if-else problem
        if (!IS_TOKEN_NONE(accept_token(ctx, TOK_KW_ELSE))) {
            if (parse_statement(ctx, &stmt_false)) {
                accept_token(ctx, TOK_SEMICOLON);
            }
        }
    } else {
        expect_token(ctx, TOK_SEMICOLON);
    }
    
    SourceLocation sl = parsed_source_location(ctx, s);
    act_on_stmt_if(sl, condition, stmt_true, stmt_false, result);
    context_scope_pop(ctx);
    return true;
}

bool parse_stmt_labeled(Context *ctx, ASTBase **result) {
    Context s = snapshot(ctx);
    ASTIdent *label = NULL;
    if (!parse_ident(ctx, &label)) { return false; }
    
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_COLON))) { goto fail_parse; }
    
    ASTStatement *stmt = (ASTStatement*)expect_node(ctx, (ParseFn)parse_statement,
                                                    "expected statement after label");
    act_on_stmt_labeled(parsed_source_location(ctx, s), label, stmt, (ASTStmtLabeled**)result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_stmt_compound(Context *ctx, ASTBlock **result) {
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_LBRACE))) { return false; }
    
    Context s = snapshot(ctx);
    List *stmts = NULL;
    
    context_scope_push(ctx);
    
    ASTBase *stmt = NULL;
    while (parse_block_item(ctx, &stmt)) {
        list_append(&stmts, stmt);
    }
    
    expect_token(ctx, TOK_RBRACE);
    
    act_on_block(parsed_source_location(ctx, s), stmts, result);
    context_scope_pop(ctx);
    return true;
}

bool parse_stmt_selection(Context *ctx, ASTBase **result) {
    // SWITCH
    return parse_stmt_if(ctx, (ASTStmtIf**)result);
}

bool parse_stmt_iteration(Context *ctx, ASTBase **result) {
    return false;
}

bool parse_stmt_jump(Context *ctx, ASTBase **result) {
    Token t = accept_token(ctx, TOK_KW_GOTO);
    if (IS_TOKEN_NONE(t)) t = accept_token(ctx, TOK_KW_CONTINUE);
    if (IS_TOKEN_NONE(t)) t = accept_token(ctx, TOK_KW_BREAK);
    if (IS_TOKEN_NONE(t)) return parse_stmt_return(ctx, (ASTStmtReturn**)result);
    
    // Statement keyword was one of [GOTO | CONTINUE | BREAK]
    ASTIdent *label = NULL;
    parse_ident(ctx, &label);
    expect_token(ctx, TOK_SEMICOLON);
    
    act_on_stmt_jump(t.location, t, label, (ASTStmtJump**)result);
    return true;
}

bool parse_statement(Context *ctx, ASTBase **result) {
    // NOTE(bloggins): parse_stmt_declaration intentionally omitted.
    // it is not actually a "statement" and will be removed.
    return
    parse_stmt_labeled(ctx, result) ||
    parse_stmt_compound(ctx, (ASTBlock**)result) ||
    parse_stmt_expression(ctx, (ASTStmtExpr**)result) ||
    parse_stmt_selection(ctx, result) ||
    parse_stmt_iteration(ctx, result) ||
    parse_stmt_jump(ctx, result);
}

/* ====== Misc (Needs categorization) ====== */
#pragma mark Misc

bool parse_block_item(Context *ctx, ASTBase **result) {
    return
    parse_pp_directive(ctx, (ASTPPDirective**)result) ||
    parse_stmt_declaration(ctx, (ASTStmtDecl**)result) ||
    parse_statement(ctx, result);
}

bool parse_end(Context *ctx) {
    expect_token(ctx, TOK_END);
    return true;
}

bool parse_toplevel(Context *ctx, ASTTopLevel **result) {
    Context s = snapshot(ctx);
    List *stmts = NULL;
    
    Scope *scope = context_scope_push(ctx);
    
    ASTBase *stmt = NULL;
    while (parse_pp_directive(ctx, (ASTPPDirective**)&stmt) ||
           parse_decl_external(ctx, (ASTDeclaration**)&stmt) ||
           (ctx->parse_mode.allow_toplevel_expressions && parse_stmt_expression(ctx, (ASTStmtExpr**)&stmt))) {
        // Not every successful parse contributes an AST node
        if (stmt == NULL) {
            continue;
        }
        
        if (AST_IS(stmt, AST_TOPLEVEL)) {
            // Probably from an include. Merge
            List_FOREACH(ASTBase *, node, ((ASTTopLevel*)stmt)->definitions, {
                if (AST_IS(node, AST_STMT_DECL)) {
                    scope_declaration_add(scope, ((ASTStmtDecl*)node)->declaration);
                } else {
                    scope_declaration_add(scope, (ASTDeclaration*)node);
                }
            
                list_append(&stmts, node);
            })
        } else {
            list_append(&stmts, stmt);
        }
    }
    
    if (!parse_end(ctx)) {
        context_scope_pop(ctx);
        return false;
    }
    
    act_on_toplevel(parsed_source_location(ctx, s), scope, stmts, result);
    
    // This is safe because there's yet another scope above this -
    // the compiler's builtins scope
    context_scope_pop(ctx);
    
    return true;
}


/* ====== Preprocessor ====== */
#pragma mark Preprocessor

/* Because things could get... meta */
typedef bool (*PPParseFn(Context *ctx, void **result));

bool parse_pp_pragma(Context *ctx, ASTPPPragma **result) {
    Context s = snapshot(ctx);
    
    // This is a hack to make sure we only parse pragma directives on the line
    // the pragma was defined on.
    uint32_t lineof_directive = ctx->line;
    
    // TODO(bloggins): This is not valid C syntax for pragma. We do this for
    // now until we have a more complete pre-processor.
    ASTIdent *arg1 = NULL;
    if (!parse_ident(ctx, &arg1) || arg1->base.location.line != lineof_directive) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "argument expected after pragma");
    }
    
    if (!spelling_streq(arg1->base.location.spelling, "CLITE")) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "unsupported argument '%s' after pragma. Only 'CLITE' is supported",
                    spelling_cstring(arg1->base.location.spelling));
    }
    
    // This is the actual argument to be used for directives
    ASTIdent *arg2 = NULL;
    if (!parse_ident(ctx, &arg2) || arg2->base.location.line != lineof_directive) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "second argument expected after pragma");
    }
    
    // Anything after the 2nd argument will have to be taken care of by the
    // action
    Token chunk = lexer_lex_chunk(ctx, '\n', '\\');
    
    act_on_pp_pragma(parsed_source_location(ctx, s), arg1, arg2, chunk,
                     (ASTPPPragma**)result);
    return true;
}

bool parse_pp_run(Context *ctx, ASTBase **result) {
    Token chunk = lexer_lex_chunk(ctx, '\n', '\\');
    if (chunk.kind != TOK_CHUNK) {
        diag_emit(DIAG_FATAL, ERR_PARSE, &chunk.location, "parse error parsing run directive");
    }
    
    act_on_pp_run(chunk.location, ctx, chunk, '\\', (ParseFn)parse_expression, result);
    return true;
}

bool parse_pp_include(Context *ctx, ASTBase **result) {
    Context s = snapshot(ctx);
    
    bool system_include = false;
    const char *include_file = NULL;
    if (!IS_TOKEN_NONE(accept_token(ctx, TOK_LANGLE))) {
        Token chunk = lexer_lex_chunk(ctx, '>', 0);
        if (chunk.kind != TOK_CHUNK) {
            diag_emit(DIAG_ERROR, ERR_PARSE, &chunk.location, "syntax error after #include");
        }
        Spelling sp_chunk = chunk.location.spelling;
        sp_chunk.end--; // Get rid of the '>'
        include_file = strdup(spelling_cstring(sp_chunk));
        system_include = true;
    } else {
        ASTExprString *str = (ASTExprString*)expect_node(ctx, (ParseFn)parse_expr_string,
                                         "syntax error after #include");
        include_file = strdup(spelling_cstring(AST_BASE(str)->location.spelling));
        if (include_file == NULL || include_file[0] == 0) {
            diag_emit(DIAG_ERROR, ERR_PARSE, &AST_BASE(str)->location,
                        "include directive must name a file");
        }
    }
    
    include_file = strdup(include_file);
    SourceLocation sl = parsed_source_location(ctx, s);
    act_on_pp_include(sl, include_file, system_include,
                      ctx->active_scope, result);
    
    return true;
}

bool parse_pp_define(Context *ctx, ASTPPDefinition **result) {
    Context s = snapshot(ctx);
    
    ASTIdent *name = (ASTIdent*)expect_node(ctx, (ParseFn)parse_ident,
                                            "expected identifier after #define");
    
    Token chunk = lexer_lex_chunk(ctx, '\n', '\\');
    if (chunk.kind != TOK_CHUNK) {
        diag_emit(DIAG_FATAL, ERR_PARSE, &chunk.location, "parse error parsing run directive");
    }
    
    act_on_pp_define(parsed_source_location(ctx, s), name, chunk.location.spelling,
                     result);
    return true;
}

bool parse_pp_ifdef(Context *ctx, ASTPPIf **result) {
    Context s = snapshot(ctx);
    
    ASTIdent *ident = NULL;
    if (!parse_ident(ctx, &ident)) { goto fail_parse; }
    
    // TODO(bloggins): action
    //act_on_pp_ifndef(parsed_source_location(ctx, s), ident, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;

}

bool parse_pp_ifndef(Context *ctx, ASTPPIf **result) {
    Context s = snapshot(ctx);
    
    ASTIdent *ident = NULL;
    if (!parse_ident(ctx, &ident)) { goto fail_parse; }
    
    act_on_pp_ifndef(parsed_source_location(ctx, s), ident, result);
    return true;
    
fail_parse:
    restore(ctx, s);
    return false;
}

bool parse_pp_if(Context *ctx, ASTPPIf **result) {
    Context s = snapshot(ctx);
    
    //
    // TODO(bloggins): just parse an expression and interpret it
    //
    
    accept_token(ctx, TOK_BANG);
    
    ASTIdent *kw = (ASTIdent*)expect_node(ctx, (ParseFn)parse_ident, "expected identifier");
    if (!spelling_streq(kw->base.location.spelling, "defined")) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "expected 'defined' (temporary restriction)");
    }
    
    expect_token(ctx, TOK_LPAREN);
    
    /*ASTIdent *ident = (ASTIdent*)*/expect_node(ctx, (ParseFn)parse_ident, "expected identifier after 'defined'");
    
    expect_token(ctx, TOK_RPAREN);
    
    lexer_lex_chunk(ctx, '\n', 0);
    
    
    // TODO(bloggins): action
    return true;
}

bool parse_pp_else(Context *ctx, ASTPPIf **result) {
    lexer_lex_chunk(ctx, '\n', 0);
    
    // TODO(bloggins): action
    return true;
}

bool parse_pp_endif(Context *ctx, ASTPPIf **result) {
    lexer_lex_chunk(ctx, '\n', 0);
    
    // TODO(bloggins): action
    return true;
}

bool parse_pp_warning(Context *ctx, ASTPPDirective **result) {
    Context s = snapshot(ctx);
    
    Token t = lexer_lex_chunk(ctx, '\n', '\\');
    SourceLocation sl = parsed_source_location(ctx, s);
    sl.spelling.start = 0;
    sl.spelling.end = 0;
    
    // TODO(bloggins): move to action
    diag_emit(DIAG_WARNING, ERR_NONE, &sl, "%s", t.kind == TOK_CHUNK ?
                spelling_cstring(t.location.spelling) : "");
    
    return true;
}

bool parse_pp_directive(Context *ctx, ASTPPDirective **result) {
    // NOTE(bloggins): Eventually the preprocessor will do something fancy, but
    // for right now preprocessor tokens will just be inserted into the AST
    Context s = snapshot(ctx);
    
    if (IS_TOKEN_NONE(accept_token(ctx, TOK_HASH))) { goto fail_parse; }
    
    bool saved_lex_mode = ctx->lex_mode.lex_keywords_as_identifiers;
    ctx->lex_mode.lex_keywords_as_identifiers = true;
    
    ASTIdent *directive = NULL;
    if (!parse_ident(ctx, &directive)) { goto fail_parse; }
    
    Spelling directive_sp = AST_BASE(directive)->location.spelling;
    PPParseFn *parse_fn = NULL;
    if (spelling_streq(directive_sp, "pragma")) {
        parse_fn = (PPParseFn*)parse_pp_pragma;
    } else if (spelling_streq(directive_sp, "run")) {
        parse_fn = (PPParseFn*)parse_pp_run;
    } else if (spelling_streq(directive_sp, "include")) {
        parse_fn = (PPParseFn*)parse_pp_include;
    } else if (spelling_streq(directive_sp, "if")) {
        parse_fn = (PPParseFn*)parse_pp_if;
    } else if (spelling_streq(directive_sp, "ifdef")) {
        parse_fn = (PPParseFn*)parse_pp_ifdef;
    } else if (spelling_streq(directive_sp, "ifndef")) {
        parse_fn = (PPParseFn*)parse_pp_ifndef;
    } else if (spelling_streq(directive_sp, "define")) {
        parse_fn = (PPParseFn*)parse_pp_define;
    } else if (spelling_streq(directive_sp, "else")) {
        parse_fn = (PPParseFn*)parse_pp_else;
    } else if (spelling_streq(directive_sp, "endif")) {
        parse_fn = (PPParseFn*)parse_pp_endif;
    } else if (spelling_streq(directive_sp, "warning")) {
        parse_fn = (PPParseFn*)parse_pp_warning;
    } else if (spelling_streq(directive_sp, "break")) {
        // TODO(bloggins): in release mode, do something different
        // like yield to an environment-defined break routine
        asm("int $3");
        goto done;
    }
    
    if (parse_fn == NULL) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "unrecognized preprocessor directive '%s'",
                    spelling_cstring(directive->base.location.spelling));
    }
    
    if (!parse_fn(ctx, (void**)result)) {
        SourceLocation sl = parsed_source_location(ctx, s);
        diag_emit(DIAG_ERROR, ERR_PARSE, &sl, "invalid syntax following preprocessor directive");
    }

done:
    ctx->lex_mode.lex_keywords_as_identifiers = saved_lex_mode;
    return true;
    
fail_parse:
    restore(ctx, s);
    ctx->lex_mode.lex_keywords_as_identifiers = saved_lex_mode;
    return false;
}

/* ====== Public API ====== */
#pragma mark Public API

void parser_parse(Context *ctx) {
    if (!parse_toplevel(ctx, (ASTTopLevel**)&ctx->ast)) {
        exit(ERR_PARSE);
    }
    
    // NOTE(bloggins): Sanity check
    if (ctx->pos - ctx->buf < ctx->buf_size) {
        diag_emit(DIAG_ERROR, ERR_PARSE, NULL, "unexpected end of input", ctx->file);
    }
}
