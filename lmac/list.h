//
//  list.h
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_list_h
#define lmac_list_h

typedef struct List {
    void *item;
    
    struct List *next;
} List;

#define List_FOREACH(type, var_name, list, block)                       \
do {                                                                    \
List *list_copy = list;                                                 \
while((list_copy) != NULL) {                                            \
type var_name = (type)(list_copy)->item;                                \
assert(var_name != NULL);                                               \
list_copy = list_copy->next;                                            \
block                                                                   \
}                                                                       \
} while(0);


// TODO(bloggins): list_destroy();

/* Adds the item to the end of the given list. Creates the list if it doesn't
 * yet exist
 */
void list_append(List **list, void *item);

#endif
