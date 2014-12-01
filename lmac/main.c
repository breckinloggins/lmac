//
//  main.c
//  lmac
//
//  Created by Breckin Loggins on 12/1/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>

#define ERR_NONE                0
#define ERR_USAGE               1
#define ERR_FILE_NOT_FOUND      2

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
    
    return ERR_NONE;
}
