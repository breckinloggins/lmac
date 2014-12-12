//
//  4_meta.c
//  lmac
//
//  Created by Breckin Loggins on 12/8/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// Test #run and other weird things

// The following two run commands should work but do... nothing. Nothing at all
//#run
//#run    // Like I said, nothing at all, though this will still try to "run" the comment

/* -- TODO(bloggins): This doesn't work because the lexer doesn't respect 
 * line continuations in comments
#run // This is an example of a multiline chunk. It will do nothing. because \
it is a comment, but it will still parse all of it.                             \
At least, I think it will.
*/
 
$32 a = #run 42
// TODO(bloggins): This semicolon after the line has to be there to placate the
// parser, but we should be able to remove it with automatic semicolon insertion
;

$32 printf();

$32 main() {
    printf("Hello, World!\n");
    return a;
}
