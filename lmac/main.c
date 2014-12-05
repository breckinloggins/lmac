//
//  main.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#include <unistd.h>
#include <sys/syslimits.h>
#include <stdarg.h>

// INFO(bloggins): I normally don't use any kind of "hungarian" prefixes
// on variables, but I make an exception for globals. They should be rare
// and they should stand out.
global_variable Context g_ctx;

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

int run_cmd(const char *fmt, ...) {
    int ret = 0;
    va_list ap;
    
    va_start(ap, fmt);
    
    char *cmd = NULL;
    vasprintf(&cmd, fmt, ap);
    assert(cmd && "cmd must not be null");
    va_end(ap);
    
    fprintf(stderr, "[c-compile] %s\n", cmd);
    ret = system(cmd);
    free(cmd);
    
    return ret;
}

const char *lookup_cmd(const char *cmd) {
    // TODO(bloggins): WARNING - not thread safe
    static char path[PATH_MAX];
    
    FILE *fp;
    char *which_cmd = NULL;
    asprintf(&which_cmd, "%s %s", "/usr/bin/which", cmd);
    fp = popen(which_cmd, "r");
    free(which_cmd);
    
    bzero(path, PATH_MAX);
    fgets(path, PATH_MAX-1, fp);
    
    fclose(fp);
    
    if (path[0] == 0) {
        return NULL;
    }
    
    // No newlines
    for (int i = 0; i < PATH_MAX; i++) {
        if (path[i] == 0) {
            break;
        }
        
        if (path[i] == '\n') {
            path[i] = ' ';
        }
    }
    
    return path;
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
    
    // TODO(bloggins): Extract to a driver struct
    const char *cc_path = lookup_cmd("cc");
    if (cc_path == NULL) {
        diag_printf(DIAG_ERROR, NULL, "can't find c compiler (cc)");
        return ERR_CC;
    }
    
    // TODO(bloggins): this is probably not the most memory efficient thing we could do
    FILE *fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    g_ctx.file = file;
    g_ctx.buf = (uint8_t *)malloc(fsize + 1);
    fread(g_ctx.buf, fsize, 1, fp);
    fclose(fp);
    
    // Ensure null-termination
    g_ctx.buf[fsize] = 0;
    g_ctx.pos = g_ctx.buf;
    g_ctx.line = 1;
    
    parser_parse(&g_ctx);
    
    // NOTE(bloggins): Sanity check
    if (g_ctx.pos - g_ctx.buf < fsize) {
        diag_printf(DIAG_FATAL, NULL, "unexpected end of input", file);
        return ERR_LEX;
    }
    
    PrintCtx pctx = {};
    ast_visit((ASTBase*)g_ctx.ast, print_visitor, &pctx);
    
    analyzer_analyze(g_ctx.ast);
    
    FILE *fout = fopen("tmp_out.c", "w");
    codegen_generate(fout, g_ctx.ast);
    fflush(fout);
    fclose(fout);

    // NOTE(bloggins): We aren't freeing anything in the global context. There's no point
    // since the OS does that for us anyway and we don't want to take any longer to exit
    // than we need to.
    return run_cmd("%s -o tmp_out tmp_out.c", cc_path);
}
