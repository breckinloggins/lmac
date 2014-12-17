//
//  spelling.c
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "clite.h"

size_t spelling_strlen(Spelling spelling) {
    return (spelling.end - spelling.start);
}

bool spelling_streq(Spelling spelling, const char *str) {
    size_t sp_len = spelling_strlen(spelling);
    if (sp_len != strlen(str)) {
        return false;
    }
    
    const char *c = str;
    while (*c) {
        size_t off = c - str;
        if (spelling.start + off > spelling.end) {
            // Ran off the end of the token, can't be equal
            return false;
        }
        
        char tc = (char)*(spelling.start + off);
        if (*c != tc) {
            return false;
        }
        
        ++c;
    }
    
    return true;

}

bool spelling_equal(Spelling spelling1, Spelling spelling2) {
    if (spelling1.start == spelling2.start && spelling1.end == spelling2.end) {
        // Trivially equal
        return true;
    }
    
    size_t sp1_len = spelling_strlen(spelling1);
    size_t sp2_len = spelling_strlen(spelling2);
    
    if (sp1_len != sp2_len) {
        return false;
    }
    
    for (int i = 0; i < sp1_len; i++) {
        if (spelling1.start[i] != spelling2.start[i]) {
            return false;
        }
    }
    
    return true;
}

void spelling_fprint(FILE *f, Spelling spelling) {
    // TODO(bloggins):
    //  1. Break this out into sprintf, asprintf, etc. functions
    //  2. Control whether to escape or not
    for (char *cp = (char*)spelling.start; cp < (char*)spelling.end; cp++) {
        if (*cp == '\n') {
            fprintf(f, "\\n");
        } else if (*cp == '\t') {
            fprintf(f, "\\t");
        } else {
            fprintf(f, "%c", *cp);
        }
    }
}

void spelling_line_fprint(FILE *f, Spelling spelling) {
    if (spelling.start == NULL || spelling.end == NULL) {
        return;
    }
    
    const char *buf = (const char *)spelling.ctx->buf;
    
    const char *line_begin = (const char *)spelling.end;
    const char *line_end = (const char *)spelling.end;
    
    if (*line_begin == '\n') {
        --line_begin;
    }
    
    while (line_begin != buf && *line_begin != '\n') {
        if (--line_begin <= buf) {
            line_begin = buf;
            break;
        }
    }
    
    while (*line_end != '\n') {
        if (++line_end >= (buf + spelling.ctx->buf_size)) {
            line_end = buf + spelling.ctx->buf_size - 1;
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
        if (ch == (const char *)spelling.end) {
            fputc('^', f);
            break;
        } else {
            fputc('~', f);
        }
    }
    fprintf(f, "\n");
}

const char *spelling_cstring(Spelling spelling) {
    // TODO(bloggins): This is not thread safe!
    static char *cstr = NULL;
    static size_t cstr_len = 0;
    
    size_t sp_len = spelling_strlen(spelling);
    if (sp_len == 0) {
        return "";
    }
    
    if (cstr == NULL || cstr_len == 0) {
        cstr = (char *)malloc(sp_len + 1);
    } else if (cstr_len < sp_len) {
        // Need to make more space for this one
        free(cstr);
        cstr = (char *)malloc(sp_len + 1);
    }
   
    cstr_len = sp_len;
    for (int i = 0; i < cstr_len; i++) {
        cstr[i] = (char)spelling.start[i];
    }
    cstr[cstr_len] = 0;
    return cstr;
}
