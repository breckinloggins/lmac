//
//  interp.c
//  lmac
//
//  Created by Breckin Loggins on 12/9/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#define INTERP_ERROR(sl, ...)                                              \
diag_printf(DIAG_ERROR, sl, __VA_ARGS__);                               \
exit(ERR_INTERPRET);

typedef struct InterpNodeData {
    VisitFn visit;
} InterpNodeData;

static InterpNodeData interp_data[AST_LAST] = {0};
static bool interp_data_initialized = false;

int default_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    assert(node && "node must be valid");
    SourceLocation sl = node->location;
    INTERP_ERROR(&sl, "cannot interpret node kind %s", ast_get_kind_name(node->kind));
}

int dispatch_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    return interp_data[node->kind].visit(node, phase, ctx);
}

void initialize_interp_data() {
    for (int i = 0; i < (int)AST_LAST; i++) {
        interp_data[i].visit = default_visitor;
    }
}

bool interp_interpret(ASTBase *node, ASTBase **result) {
    if (!interp_data_initialized) {
        // TODO(bloggins): Not thread safe
        initialize_interp_data();
        interp_data_initialized = true;
    }
    
    return ast_visit(node, dispatch_visitor, NULL);
}
