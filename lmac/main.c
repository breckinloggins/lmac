//
//  main.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <unistd.h>

#include "clite.h"

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
    
    char indent[pctx->indent_level + 1];
    for (int i = 0; i < pctx->indent_level; i++) {
        indent[i] = '\t';
    }
    indent[pctx->indent_level] = 0;
    
    FILE *f = stderr;
    fprintf(f, "%s%d:%s", indent, node->location.line, ast_get_kind_name(node->kind));
    if (node->kind == AST_DEFN) {
        size_t content_size = node->location.range_end - node->location.range_start;
        char content[content_size + 1];
        char *pc = (char *)node->location.range_start;
        for (int i = 0; i < content_size; i++) {
            content[i] = *pc++;
            if (content[i] == '\n') content[i] = '$';
            if (content[i] == '\t') content[i] = '$';
        }
        content[content_size] = 0;
        
        fprintf(f, " <%s>\n", content);
    }
    fprintf(f, "\n");
    
    ++pctx->indent_level;
    
    return VISIT_OK;
}

int main(int argc, const char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: lmac <file>\n");
        return ERR_USAGE;
    }
    
    // Make sure the file exists
    const char *file = argv[1];
    if (access(file, R_OK) == -1) {
        fprintf(stderr, "error: input file not found (%s)\n", file);
        return ERR_FILE_NOT_FOUND;
    }
    
    // TODO(bloggins): this is probably not the most memory efficient thing we could do
    FILE *fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
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
        fprintf(stderr, "Unexpected end of input\n");
        return ERR_LEX;
    }
    
    PrintCtx pctx = {};
    ast_visit((ASTBase*)g_ctx.ast, print_visitor, &pctx);
    
    // NOTE(bloggins): We aren't freeing anything in the global context. There's no point
    // since the OS does that for us anyway and we don't want to take any longer to exit
    // than we need to.
    return ERR_NONE;
}
