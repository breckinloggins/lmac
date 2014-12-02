//
//  Tokens.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef TOKEN
#define TOKEN(kind)
#endif

/* Should ALWAYS be first */
TOKEN(TOK_UNKOWN)

/* Everything that isn't lexed as a specific token is
 * characterized as an "identifier". This includes types,
 * variables, and functions. This keeps the lexer or more sane and flexible
 * and moves things like "can't assign a type to a variable"
 * into the semantic analyzer
 */
TOKEN(TOK_IDENT)

/* Only integer literal for now */
TOKEN(TOK_NUMBER)
    
/* Tokens for "structural symbols" like '{' and ';' */
TOKEN(TOK_SEMICOLON)
TOKEN(TOK_LBRACE)
TOKEN(TOK_RBRACE)
TOKEN(TOK_LPAREN)
TOKEN(TOK_RPAREN)
TOKEN(TOK_EQUALS)

/* Math and logic symbols */
TOKEN(TOK_PLUS)
    
/* Our tokenizer is whitespace and comment preserving
 * to better map input code to generated code
 */
TOKEN(TOK_WS)
TOKEN(TOK_COMMENT)
    
/* Annotation token for end of input */
TOKEN(TOK_END)
    
/* Should ALWAYS be last */
TOKEN(TOK_LAST)

#ifdef TOKEN
#undef TOKEN
#endif