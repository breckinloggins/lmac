//
//  ast.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef AST
#define AST(ucase_name, name, struct)
#endif

/* Should ALWAYS be first */
AST(UNKNOWN, unknown, ASTBase)

AST(TOPLEVEL, toplevel, ASTTopLevel)
AST(DEFN, defn, ASTDefn)
AST(IDENT, ident, ASTIdent)

/* Should ALWAYS be last */
AST(LAST, last, ASTBase)

#ifdef AST
#undef AST
#endif