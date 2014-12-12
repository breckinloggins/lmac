//
//  context.c
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"
#include <unistd.h>

Scope *context_scope_push(Context *ctx) {
    Scope *s = scope_create();
    
    Scope *prev = ctx->active_scope;
    ctx->active_scope = s;
    
    if (prev != NULL) {
        scope_child_add(prev, s);
    }
    
    return s;
}

Scope *context_scope_pop(Context *ctx) {
    Scope *prev = ctx->active_scope;
    assert(prev && "cannot pop empty scope");
    assert(prev->parent && "cannot pop last scope");
    
    ctx->active_scope = prev->parent;
    return prev->parent;
}

void context_load_file(Context *ctx, const char *filename) {
    assert(filename);
    assert(!ctx->file && "context shouldn't already have a file");
    
    if (access(filename, R_OK) == -1) {
        diag_printf(DIAG_ERROR, NULL, "input file not found (%s)", filename);
        exit(ERR_FILE_NOT_FOUND);
    }
    
    // TODO(bloggins): this is probably not the most memory efficient thing we could do
    FILE *fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    ctx->buf = (uint8_t *)malloc(fsize + 1);
    fread(ctx->buf, fsize, 1, fp);
    fclose(fp);
    
    // Ensure null-termination
    ctx->file = filename;
    ctx->buf[fsize] = 0;
    ctx->buf_size = fsize;
    ctx->pos = ctx->buf;
    ctx->line = 1;
}
