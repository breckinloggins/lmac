//
//  scope.h
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_scope_h
#define lmac_scope_h

#include "ast.h"

typedef struct Scope {
    struct Scope *parent;
    List *children;
    
    List *declarations;
    List *labels;
} Scope;

Scope *scope_create();
void scope_child_add(Scope *scope, Scope *child);
void scope_declaration_add(Scope *scope, ASTDeclaration *decl);
void scope_label_add(Scope *scope, ASTBase *label);
ASTDeclaration *scope_lookup_declaration(Scope *scope, Spelling name, bool search_parents);

void scope_dump(Scope *scope);
void scope_fdump(FILE *f, Scope *scope);

#endif
