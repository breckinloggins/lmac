// This is a comment
/* This is also a comment */

/*
 * This is
 * a multiline comment
 */

// TODO(bloggins): Test what happens when a multiline comment isn't closed
//                  or a single line comment occurs at the end of a file
//                  without a newline

void foo() { }

int a = 3;
int b = 43;

void bar() {
    int c = 3;
}

int d = 513;

int main() {
    int c = 5;

    int d = a + b + c + d;
    return d;
}
