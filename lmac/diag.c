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

const char *diag_get_name(DiagKind kind) {
    return g_diag_kind_names[kind];
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
        
        fprintf(f, " (%s:%d)", file_name, loc->line);
    }
    fprintf(f, ": ");
    vfprintf(f, fmt, ap);
    fprintf(f, "\n");
    
    va_end(ap);
}

