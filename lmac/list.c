//
//  list.c
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "list.h"

#include <assert.h>
#include <stdlib.h>

void list_append(List **list, void *item) {
    assert(item && "item is NULL");
    
    List *l = NULL;
    if (*list == NULL) {
        *list = calloc(1, sizeof(List));
        l = *list;
    } else {
        l = *list;
        while (l->next != NULL) {
            l = l->next;
        }
        
        l->next = calloc(1, sizeof(List));
        l = l->next;
    }
    
    l->item = item;
}
