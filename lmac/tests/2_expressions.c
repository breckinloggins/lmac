// Test of expression types
#pragma CLITE fcg_explicit_parens  /* make the code generator group all expressions explicitly */

// TODO(bloggins): Move this into the prelude
$$() int = $32;
$$() void = $();

int a = 1;
int b = 2;
int c = 4;

void foo() { }

int main() {

    int add2 = a + b;
    int add3 = a + b + c;

    int lshift = add2 << 2;
    int rshift = add2 >> 2;
    
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