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

int do_repl() {
    FILE *f_in = stdin;
    FILE *f_out = stdout;
    
    fprintf(f_out, "Cx REPL v.whatever\n");
    fprintf(f_out, "\n");
    
    Scope *outer_scope = scope_create();
    
    while (true) {
        fprintf(f_out, "> ");
        char *user_input = NULL;
        size_t input_size = 0;
        size_t chars_read = 0;
        
        if ((chars_read = getline(&user_input, &input_size, f_in)) == -1) {
            break;
        }
        
        // Be nice to them
        user_input[chars_read] = ';';
        
        Context *ctx = context_create();
        ctx->active_scope = outer_scope;
        ctx->file = "<repl>";
        ctx->parse_mode.allow_toplevel_expressions = true;
        
        ctx->line = 1;
        ctx->buf = (uint8_t*)user_input;
        ctx->pos = ctx->buf;
        
        // TODO: use setjmp/longjmp so we can quickly error after parsing problems
        // but not quit the repl
        // http://en.wikipedia.org/wiki/Setjmp.h
        

        parser_parse(ctx);
        
        ASTBase *result = NULL;
        interp_interpret(ctx->ast, &result);
        if (result != NULL) {
            ct_dump(result);
        }
        
        ct_autorelease();
        
        free(user_input);
        
        
    }
    
    return ERR_NONE;
}

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: lmac [build | run | interpret | repl] <file>\n");
        return ERR_USAGE;
    }
    
    const int ACTION_BUILD = 0;
    const int ACTION_RUN = 1;
    const int ACTION_INTERPRET = 2;
    const int ACTION_REPL = 3;
    
    const char *action_str = argv[1];
    int action;
    bool needs_file = false;
    if (!strcmp(action_str, "build")) {
        action = ACTION_BUILD;
        needs_file = true;
    } else if (!strcmp(action_str, "run")) {
        action = ACTION_RUN;
        needs_file = true;
    } else if (!strcmp(action_str, "interpret")) {
        action = ACTION_INTERPRET;
        needs_file = true;
    } else if (!strcmp(action_str, "repl")) {
        action = ACTION_REPL;
        needs_file = false;
    } else {
        diag_printf(DIAG_FATAL, NULL, "unknown action '%s'", action_str);
        return ERR_USAGE;
    }
    
    atexit(exit_handler);
    
    // Shortcut to the repl if that action is specified
    int res = ERR_NONE;
    if (action == ACTION_REPL) {
        res = do_repl();
        goto finish;
    }
    
    // Make sure the file exists
    const char *file = NULL;
    if (needs_file) {
        if (argc != 3) {
            diag_printf(DIAG_FATAL, NULL, "you must specify a file when you %s", action_str);
            return ERR_USAGE;
        }
        file = argv[2];
    }
    
    Context *ctx = context_create();
    ctx->file = file;
    
    if (ctx->file != NULL) {
        const char *filename = ctx->file;
        ctx->file = NULL;
        context_load_file(ctx, filename);
    }
    
    // Initialize top scope, builtins, etc. here before parsing
    context_scope_push(ctx);
    
    parser_parse(ctx);
    
    // Don't keep parse context around after the full parse tree is created
    ctx->active_scope = NULL;
    
    if (action == ACTION_BUILD || action == ACTION_RUN) {
        analyzer_analyze(ctx->ast);
        res = run_compile(ctx, action == ACTION_RUN);
    } else if (action == ACTION_INTERPRET) {
        ASTBase *result = NULL;
        res = interp_interpret(ctx->ast, &result);
        if (res) {
            ct_dump(result);
        } else {
            fprintf(stderr, "%s\n", "interpreter failed");
        }
    }

finish:
    ct_autorelease();
    return res;
}
