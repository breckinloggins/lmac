//
//  c_prelude.h
//  lmac
//
//  Created by Breckin Loggins on 12/10/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// TODO(bloggins): We get an error when we try to put this here
// #pragma CLITE fcg_explicit_parens  /* make the code generator group all expressions explicitly */

$$() void = $();
$$() char = $c8;
$$() int = $i32;

// TODO: should be able to parse
// int printf(const char *, ...);
int printf(const char * fmt, ...);
