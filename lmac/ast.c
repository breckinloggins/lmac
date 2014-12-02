//
//  ast.c
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <stdbool.h>

#include "clite.h"

// NOTE(bloggins): We're doing lots of small allocations here. It might
//                  be worth it to use a pool allocator or other optimization
//                  later.

global_variable const char *g_ast_names[] = {
#   define AST(kind, ...) "AST_"#kind ,
#   include "ast.def.h"
};

const char *ast_get_kind_name(ASTKind kind) {
    return g_ast_names[kind];
}


ASTBase *ast_create(ASTKind kind, size_t size) {
    ASTBase *node = (ASTBase *)calloc(1, size);
    node->kind = kind;
    
    return node;
}

//
// Visitor acceptors
//

#define AST_ACCEPT_FN_NAME(kind) accept_##kind
#define AST_ACCEPT_FN(kind) int AST_ACCEPT_FN_NAME(kind)(ASTBase *node, VisitFn visit, void *ctx)

AST_ACCEPT_FN(AST_UNKNOWN) {
    return visit(node, ctx);
}

AST_ACCEPT_FN(AST_BASE) {
    return visit(node, ctx);
}

AST_ACCEPT_FN(AST_LAST) {
    return visit(node, ctx);
}

AST_ACCEPT_FN(AST_DEFN) {
    return visit(node, ctx);
}

AST_ACCEPT_FN(AST_TOPLEVEL) {
    ASTTopLevel *tl = (ASTTopLevel*)node;

    // TODO(bloggins): macro-fy
    for (ASTList *defn_list = tl->definitions; defn_list != NULL && defn_list->node != NULL; defn_list = defn_list->next) {
        ASTBase *defn = defn_list->node;
        int result = defn->accept(defn, visit, ctx);
        if (result != VISIT_OK) {
            return result;
        }
    }
    
    return visit(node, ctx);
}

//
// Creation and general
//

#define AST(kind, name, type)                               \
type *ast_create_##name() {                                 \
    type *node = (type *)ast_create(AST_##kind,             \
                                    sizeof(ASTBase) +       \
                                    sizeof(type));          \
    ((ASTBase*)node)->accept = accept_AST_##kind;           \
    return node;                                            \
}
#include "ast.def.h"

int ast_visit(ASTBase *node, VisitFn visitor, void *ctx) {
    return node->accept(node, visitor, ctx);
}

void ast_list_add(ASTList **list, ASTBase *node) {
    if (*list == NULL) {
        *list = calloc(1, sizeof(ASTList));
    }
    
    ASTList *l = *list;
    
    while (l->next != NULL) {
        l = l->next;
    }
    
    l->next = calloc(1, sizeof(ASTList));
    l->node = node;
}
