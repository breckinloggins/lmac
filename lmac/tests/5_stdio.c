//
//  5_stdio.c
//  lmac
//
//  Created by Breckin Loggins on 12/10/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "tests/c_prelude.h"
//#include "stdio.h"
#include "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/include/stdio.h"


int main(int argc, char **argv) {
    char *what = "World";
    fprintf(stderr, "Hello, %s!", what);
    return 0;
}