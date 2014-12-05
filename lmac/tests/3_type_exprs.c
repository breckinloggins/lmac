//
//  3_type_exprs.c
//  lmac
//
//  Created by Breckin Loggins on 12/4/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

$$() int = $32;
$$() float = $f32;
//$$() double = $f64;   TODO(bloggins): Why doesn't WORD_SIZE report 64 bits on my machine?
$$() uint32 = $u32;
// $$() uint64 = $u64;

int x = 4;
int main() {
    int b = 3;
    int a = ($32)(3 + 4);
    $32 c = b;
    
    float d = 3;
    //double e = 5;
    uint32 f = 21;
    //uint64 g = 33;
    
    return 0;
}