// Test of expression types
#pragma CLITE fcg_explicit_parens  /* make the code generator group all expressions explicitly */

#include "tests/c_prelude.h"

//#break
void foo() {}

int a = 1;
int b = 2;
int c = 4;

int main() {

    int add2 = a + b;
    int add3 = a + b + c;
    
    #break
    add2 = add2 + 3;
    
    int lshift = add2 << 2;
    int rshift = add2 >> 2;
    
    $c8* str = "this is a string";
    char *str2 = "and this is another string";
    
    int lt = add2 < 1;
    int gr = add2 > 1;
    int lte = add2 <= 1;
    int gte = add2 >= 1;
    
    int eq = add2 == 1;
    int neq = add2 != 1;
    
    int and = add2 & 1;
    int or = add2 | 1;
    int xor = add2 ^ 1;
    
    int land = add2 && 1;
    int lor = add2 || 1;
    
    int sub2 = a - b;
    int sub3 = a - b - c;

    int addsub3 = a + b - c;
    int addsub4 = a - b + c - c;
    
    foo();
    
    int mul2 = a * b;
    int mul3 = a * b * c;

    int div2 = a / b;
    int div3 = a / b / c;

    int mod2 = a % b;
    int mod3 = a % b % c;

    int addmul3l = a + b * c;
    int addmul3r = a * b + c;

    int subdiv3l = a - b / c;
    int subdiv3r = a / b - c;

    int parens1 = (a);
    int parens2 = (a + b);
    int parens3 = (a + b + c);
    int parens4 = a + (b + c);
    int parens5 = (a + (b + c) + c) + c;

    return 0;
}