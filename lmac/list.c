//
//  list.c
//  lmac
//
//  Created by Breckin Loggins on 12/6/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#include "list.h"
#include "ct_api.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#pragma CT Type Overrides

static CTTypeID LIST_TYPE_ID = -1;

void List_type_init(CTTypeID type_id) {
    LIST_TYPE_ID = type_id;
}

void List_dump(void *type_info, void *runtime_class, FILE *f, List *list) {
    // TODO(bloggins): This is unsafe. Use a different malloc zone for CT Types
    // so we can be more reasonably sure that we won't fault when an object
    // purports to be a CT Type but isn't
    int idx = 0;
    List_FOREACH(void *, child, list, {
        fprintf(f, "%d: ", idx++);
        
        // TODO(bloggins): ct_fdump(f) vs ct_dump(stderr)
        ct_dump(child);
    })
}

#pragma normal functions

void list_append(List **list, void *item) {
    assert(item && "item is NULL");
    
    List *l = NULL;
    if (*list == NULL) {
        *list = ct_create(LIST_TYPE_ID, 0);
        l = *list;
    } else {
        l = *list;
        while (l->next != NULL) {
            assert(l != l->next);
            l = l->next;
        }
        
        l->next = ct_create(LIST_TYPE_ID, 0);
        l = l->next;
    }
    
    l->item = item;
}

size_t list_count(List *list) {
    if (list == NULL) {
        return 0;
    }
    
    int idx = 0;
    List_FOREACH(void *, dontcare, list, {
        ++idx;
    })
    
    return idx;
}