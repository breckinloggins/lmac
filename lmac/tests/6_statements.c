//
//  6_statements.c
//  lmac
//
//  Created by Breckin Loggins on 12/12/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "tests/c_prelude.h"

int main() {
    goto start_here;
    
    printf("setting new values\n");
    
    // TODO(bloggins): can't put a label on these because they aren't statements
    // and a parsing ambiguity is created.
    int b = 3;
    int a = b + 1;

start_here:
    if (b == 3) {
        printf("b is 3\n");
    } else {
        // TODO(bloggins): This doesn't compile. Can't find start_here symbol
        //goto start_here;
    }

here1:
    if (a == 5) {
        printf("shouln't get here\n");
    } else {
        printf("a is not 5\n");
    }
    
exit_here:
    printf("a = %d\n", a);
    return 0;
}
