//
//  Tokens.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef TOKEN
#define TOKEN(kind, name)
#endif

/*
 * Token database definitions:
 * kind: the enumeration kind of the token
 * name: the name of the token as a human-readable string
 */

/* Should ALWAYS be first */
TOKEN(TOK_UNKOWN, unknown)

/* Meta token kind that signifies "no matched token" */
TOKEN(TOK_NONE, none)

/* Preprocessor symbol */
TOKEN(TOK_HASH, hash)

/* An unprocessed "chunk" of source text (usually used by the preprocessor) */
TOKEN(TOK_CHUNK, chunk)

/* Type Literal symbol */
TOKEN(TOK_DOLLAR, dollar)

/* Everything that isn't lexed as a specific token is
 * characterized as an "identifier". This includes types,
 * variables, and functions. This keeps the lexer or more sane and flexible
 * and moves things like "can't assign a type to a variable"
 * into the semantic analyzer
 */
TOKEN(TOK_IDENT, identifier)

/* Only integer literal for now */
TOKEN(TOK_NUMBER, number)

/* Language keywords */
/* TODO(bloggins): Set a "bit" on these to indicate that they are keywords */
TOKEN(TOK_KW_BEGIN, NULL)
TOKEN(TOK_KW_RETURN, return)
TOKEN(TOK_KW_GOTO, goto)
TOKEN(TOK_KW_CONTINUE, continue)
TOKEN(TOK_KW_BREAK, break)
TOKEN(TOK_KW_CONST, const)
TOKEN(TOK_KW_IF, if)
TOKEN(TOK_KW_ELSE, else)
TOKEN(TOK_KW_END, NULL)

/* Tokens for "structural symbols" like '{' and ';' */
TOKEN(TOK_COMMA, comma)
TOKEN(TOK_COLON, colon)
TOKEN(TOK_SEMICOLON, semicolon)
TOKEN(TOK_LBRACE, leftbrace)
TOKEN(TOK_RBRACE, rightbrace)
TOKEN(TOK_LPAREN, leftparen)
TOKEN(TOK_RPAREN, rightparen)
TOKEN(TOK_EQUALS, equals)
TOKEN(TOK_DOUBLE_QUOTE, double_quote)
TOKEN(TOK_QUOTE, quote)
TOKEN(TOK_DOT, dot)
TOKEN(TOK_DOT_DOT, dot_dot)
TOKEN(TOK_ELLIPSIS, ellipsis)

/* Math and logic symbols */
/* NOTE(bloggins): These should be named by their lexical form and NOT their
 * logical function! It encodes less implicit semantic information into the
 * front-end
 */
TOKEN(TOK_PLUS, plus)
TOKEN(TOK_MINUS, minus)
TOKEN(TOK_STAR, star)
TOKEN(TOK_FORWARDSLASH, forwardslash)
TOKEN(TOK_PERCENT, percent)
TOKEN(TOK_LANGLE, leftangle)
TOKEN(TOK_RANGLE, rightangle)
TOKEN(TOK_2LANGLE, two_leftangle)
TOKEN(TOK_2RANGLE, two_rightangle)
TOKEN(TOK_BANG, bang)
TOKEN(TOK_AMP, ampersand)
TOKEN(TOK_CARET, caret)
TOKEN(TOK_PIPE, pipe)
TOKEN(TOK_LANGLE_EQUALS, leftangle_equals)
TOKEN(TOK_RANGLE_EQUALS, rightangle_equals)
TOKEN(TOK_EQUALS_EQUALS, equals_equals)
TOKEN(TOK_BANG_EQUALS, bang_equals)
TOKEN(TOK_AMP_AMP, ampersand_ampersand)
TOKEN(TOK_PIPE_PIPE, pipe_pipe)

/* Our tokenizer is whitespace and comment preserving
 * to better map input code to generated code
 */
TOKEN(TOK_WS, whitespace)
TOKEN(TOK_COMMENT, comment)
    
/* Annotation token for end of input */
TOKEN(TOK_END, end)
    
/* Should ALWAYS be last */
TOKEN(TOK_LAST, last)

#ifdef TOKEN
#undef TOKEN
#endif