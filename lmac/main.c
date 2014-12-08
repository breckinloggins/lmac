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
    if (argc != 2) {
        fprintf(stderr, "Usage: lmac <file>\n");
        return ERR_USAGE;
    }
    
    // Make sure the file exists
    const char *file = argv[1];
    if (access(file, R_OK) == -1) {
        diag_printf(DIAG_ERROR, NULL, "input file not found (%s)", file);
        return ERR_FILE_NOT_FOUND;
    }
    
    atexit(exit_handler);
    
    Context ctx = {0};
    ctx.file = file;
    
    return run_context(&ctx)->last_error;    
}
