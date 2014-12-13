//
//  interp.c
//  lmac
//
//  Created by Breckin Loggins on 12/9/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// Compile-time Code Interpreter (CI)

#include "clite.h"

#define CI_ERROR(sl, ...)                                              \
diag_printf(DIAG_ERROR, sl, __VA_ARGS__);                               \
exit(ERR_INTERPRET);

typedef struct CIContext {
    ASTBase *result;
} CIContext;

typedef struct InterpNodeData {
    VisitFn visit;
} InterpNodeData;

static InterpNodeData interp_data[AST_LAST] = {0};
static bool interp_data_initialized = false;

int default_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    assert(node && "node must be valid");
    SourceLocation sl = node->location;
    CI_ERROR(&sl, "cannot interpret node kind %s", ast_get_kind_name(node->kind));
}

int dispatch_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    return interp_data[node->kind].visit(node, phase, ctx);
}

#pragma mark Interpreter Functions

#define CI_VISIT_FN(kind, type) int ci_visit_##kind(type *node, VisitPhase phase, CIContext *ctx)

CI_VISIT_FN(AST_EXPR_NUMBER, ASTExprNumber) {
    
    ctx->result = (ASTBase*)node;
    
    return VISIT_HANDLED;
}


#pragma mark Interpreter API

void initialize_interp_data() {
    for (int i = 0; i < (int)AST_LAST; i++) {
        interp_data[i].visit = default_visitor;
    }
    
#   define CI_DATA(kind) interp_data[(kind)].visit = (VisitFn)ci_visit_##kind
    
    CI_DATA(AST_EXPR_NUMBER);
    
}

bool interp_interpret(ASTBase *node, ASTBase **result) {
    if (!interp_data_initialized) {
        // TODO(bloggins): Not thread safe
        initialize_interp_data();
        interp_data_initialized = true;
    }
    
    CIContext ctx;
    ast_visit(node, dispatch_visitor, &ctx);
    
    if (result != NULL) {
        *result = ctx.result;
    }
    return true;
}

