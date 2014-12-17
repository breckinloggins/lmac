//
//  interp_default.c
//  lmac
//
//  Created by Breckin Loggins on 12/16/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

struct ByteStream;

// Weakly-linked default visitor functions for the interpreter. They can be
// overridden in the interp.c file to provide node-specific functionality

#define AST(kindname, name, type)                                                   \
int __attribute__((weak)) ci_visit_AST_##kindname(type *node, VisitPhase phase, struct ByteStream *stream) {  \
fprintf(stderr, "don't know how to interpret node kind %s\n", ast_get_kind_name(AST_BASE(node)->kind)); \
exit(ERR_INTERPRET); \
}
#include "ast.def.h"
#undef AST


