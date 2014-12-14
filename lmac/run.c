//
//  run.c
//  lmac
//
//  Created by Breckin Loggins on 12/8/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#include <stdarg.h>
#include <libgen.h>
#include <sys/syslimits.h>

int run_cmd(const char *action, const char *fmt, ...) {
    int ret = 0;
    va_list ap;
    
    va_start(ap, fmt);
    
    char *cmd = NULL;
    vasprintf(&cmd, fmt, ap);
    assert(cmd && "cmd must not be null");
    va_end(ap);
    
    bool show_cmd = false;
    if (show_cmd) {
        fprintf(stderr, "[%s] %s\n", action, cmd);
    }
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

int run_compile(Context *ctx, bool run_program) {
    assert(ctx && "must have a valid context");
    ct_retain(ctx);
    
    // TODO(bloggins): Extract to a driver struct
    const char *cc_path = lookup_cmd("cc");
    if (cc_path == NULL) {
        diag_printf(DIAG_ERROR, NULL, "can't find c compiler (cc)");
        exit(ERR_CC);
    }
    
    const char *filename = ctx->file;
    ctx->file = NULL;
    context_load_file(ctx, filename);
    
    // Initialize top scope, builtins, etc. here before parsing
    context_scope_push(ctx);
    
    parser_parse(ctx);
    
    // Don't keep parse context around after the full parse tree is created
    ctx->active_scope = NULL;
    
    analyzer_analyze(ctx->ast);
    
    char *base_filename = strdup(basename((char*)ctx->file));
    char *out_file = NULL;
    asprintf(&out_file, "%s.c", base_filename);
    
    FILE *fout = fopen(out_file, "w");
    codegen_generate(fout, ctx->ast);
    fflush(fout);
    fclose(fout);
    
    // Remove the extension to make the obj file name
    for (size_t i = strlen(base_filename); i > 0; i--) {
        if (base_filename[i-1] == '.') {
            base_filename[i-1] = 0;
            break;
        }
    }
    char *obj_file = base_filename;
    
    // NOTE(bloggins): We aren't freeing anything in the global context. There's no point
    // since the OS does that for us anyway and we don't want to take any longer to exit
    // than we need to.
    int res = run_cmd("c-compile", "%s -o %s %s", cc_path, obj_file, out_file);
    if (res == 0 && run_program) {
        res = run_cmd("run", "./%s", obj_file);
    }
    
    ct_release(ctx);
    return res;
}


