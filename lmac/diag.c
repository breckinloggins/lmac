//
//  diag.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

#include <stdarg.h>
#include <setjmp.h>

global_variable const char *g_diag_kind_names[] = {
#   define DIAG_KIND(_, name, ...) #name,
#   include "diag.def.h"
};

void *diag_exception_env = NULL;
int diag_errno = 0;

const char *diag_get_name(DiagKind kind) {
    return g_diag_kind_names[kind];
}

void diag_fprint_line(FILE *f, SourceLocation *loc) {
    spelling_line_fprint(f, loc->spelling);
}

void diag_vfprintf(FILE *f, DiagKind kind, SourceLocation *loc, const char *fmt, va_list args) {
    
    fprintf(f, "%s", diag_get_name(kind));
    if (loc != NULL) {
        const char *file_name = "<unknown file>";
        if (loc->file != NULL) {
            file_name = loc->file;
        }
        
        fprintf(f, " (%s: line %d)", file_name, loc->line);
    }
    fprintf(f, ": ");
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    
    fprintf(f, "\n");
    
    if (loc != NULL) {
        diag_fprint_line(f, loc);
    }
    
    diag_errno = (int)kind;
}

void diag_vfemit(DiagKind kind, int error, SourceLocation* loc, FILE *f, const char *fmt, va_list args) {
    
    diag_vfprintf(f, kind, loc, fmt, args);
    if (kind == DIAG_ERROR || kind == DIAG_WARNING) {
        if (diag_exception_env != NULL) {
            longjmp((int *)diag_exception_env, error);
        }
    }
}

#pragma mark Public API

void diag_emit(DiagKind kind, int error, SourceLocation *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    
    diag_vfemit(kind, error, loc, stderr, fmt, ap);
    
    va_end(ap);
    
}

void diag_printf(DiagKind kind, SourceLocation* loc, const char *fmt, ...) {
    va_list ap;
    
    FILE *f = stderr;
    va_start(ap, fmt);
    
    diag_vfprintf(f, kind, loc, fmt, ap);
    
    va_end(ap);
    
}

