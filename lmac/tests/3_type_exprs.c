//
//  3_type_exprs.c
//  lmac
//
//  Created by Breckin Loggins on 12/4/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

$$() int = $32;

int x = 4;
int main() {
    int b = 3;
    int a = ($32)(3 + 4);
    $32 c = b;
    return 0;
}