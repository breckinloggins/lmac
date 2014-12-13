//
//  6_statements.c
//  lmac
//
//  Created by Breckin Loggins on 12/12/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "tests/c_prelude.h"

int main() {
    int b = 3;
    int a = b + 1;
    
    if (b == 3) {
        printf("b is 3\n");
    }
    
    if (a == 5) {
        printf("shouln't get here\n");
    } else {
        printf("a is not 5\n");
    }
    
    // TODO(bloggins): test goto when we have labels
    
    printf("a = %d\n", a);
    return 0;
}
