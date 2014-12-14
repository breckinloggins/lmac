//
//  main.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#include <unistd.h>

typedef struct {
    int indent_level;
} PrintCtx;

int print_visitor(ASTBase *node, VisitPhase phase, void *ctx) {
    PrintCtx *pctx = (PrintCtx*)ctx;
    if (phase == VISIT_POST) {
        --pctx->indent_level;
        return VISIT_OK;
    }
    
    FILE *f = stderr;
    ast_fprint(f, node, pctx->indent_level);
    fprintf(f, "\n");
    
    ++pctx->indent_level;
    
    return VISIT_OK;
}

void exit_handler() {
    if (diag_errno > 0) {
        // Comment out to stop breaking on error
        asm("int $3");
    }
#if 0
    PrintCtx pctx = {};
    ast_visit((ASTBase*)g_ctx.ast, print_visitor, &pctx);
    ast_visit_data_clean((ASTBase*)g_ctx.ast);
#endif
}

int main(int argc, const char * argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: lmac [build | run] <file>\n");
        return ERR_USAGE;
    }
    
    const int ACTION_BUILD = 0;
    const int ACTION_RUN = 1;
    
    const char *action_str = argv[1];
    int action;
    if (!strcmp(action_str, "build")) {
        action = ACTION_BUILD;
    } else if (!strcmp(action_str, "run")) {
        action = ACTION_RUN;
    } else {
        diag_printf(DIAG_FATAL, NULL, "unknown action '%s'", action_str);
        return ERR_USAGE;
    }
    
    // Make sure the file exists
    const char *file = argv[2];
    
    atexit(exit_handler);
    
    Context *ctx = context_create();
    ctx->file = file;
    
    int res = run_compile(ctx, action == ACTION_RUN);
    
    ct_dump(ctx->ast);
    ct_autorelease();
    return res;
}
