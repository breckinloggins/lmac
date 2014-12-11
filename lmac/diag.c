//
//  diag.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"
#include <stdarg.h>

global_variable const char *g_diag_kind_names[] = {
#   define DIAG_KIND(_, name, ...) #name,
#   include "diag.def.h"
};

int diag_errno = 0;

const char *diag_get_name(DiagKind kind) {
    return g_diag_kind_names[kind];
}

void diag_fprint_line(FILE *f, SourceLocation *loc) {
    const char *buf = (const char *)loc->ctx->buf;
    
    const char *line_begin = (const char *)loc->range_end;
    const char *line_end = (const char *)loc->range_end;
    while (*line_begin != '\n') {
        if (--line_begin <= buf) {
            line_begin = buf;
            break;
        }
    }
    
    while (*line_end != '\n') {
        if (++line_end >= (buf + loc->ctx->buf_size)) {
            line_end = buf + loc->ctx->buf_size - 1;
            break;
        }
    }
    
    if (*line_begin == '\n') ++line_begin;
    if (*line_end == '\n') --line_end;
    assert(line_end >= line_begin);
    
    // TODO(bloggins): Slow
    for (const char *ch = line_begin; ch <= line_end; ch++) {
        fputc(*ch, f);
    }
    fprintf(f, "\n");
    
    for (const char *ch = line_begin; ch <= line_end; ch++) {
        if (ch == (const char *)loc->range_end) {
            fputc('^', f);
            break;
        } else {
            fputc('~', f);
        }
    }
    fprintf(f, "\n");
}

void diag_printf(DiagKind kind, SourceLocation* loc, const char *fmt, ...) {
    va_list ap;
    
    FILE *f = stderr;
    va_start(ap, fmt);
    
    fprintf(f, "%s", diag_get_name(kind));
    if (loc != NULL) {
        const char *file_name = "<unknown file>";
        if (loc->file != NULL) {
            file_name = loc->file;
        }
        
        fprintf(f, " (%s: line %d)", file_name, loc->line);
    }
    fprintf(f, ": ");
    vfprintf(f, fmt, ap);
    fprintf(f, "\n");
    
    va_end(ap);
    
    fprintf(f, "\n");
    
    if (loc != NULL) {
        diag_fprint_line(f, loc);
    }
    
    diag_errno = (int)kind;
}

