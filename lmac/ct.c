//
//  ct.c
//  lmac
//
//  Created by Breckin Loggins on 12/13/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// "compile time" infrastructure

#include "ct.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#pragma mark Default VTABLE prototypes

void default_rtinit(CTTypeInfo *type_info, CTRuntimeClass *runtime_class);
void *default_alloc(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, size_t extra_bytes);
void default_dealloc(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj);
void default_init(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj);
void default_retain(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj);
void default_release(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj);
void default_dump(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, FILE *f, void *obj);

#pragma mark Static Registry

size_t CT_BASE_SIZES[CT_TYPE_ID_RESERVED] = {};
CTRuntimeClass *CT_RUNTIME_CLASS[CT_TYPE_ID_RESERVED] = {};

CTTypeInfo CT_TYPE_INFO[] = {
#   define CT_TYPE(type_name)     \
    { CT_TYPE_ID(type_name), #type_name, 0, NULL },
#   include "ct_types.def.h"
};

CTRuntimeClass RTC_Invalid = { 0, 0, NULL };
CTRuntimeClass RTC_Default = { 0, 0, NULL };

#define CT_TYPE(type_name) CTRuntimeClass _RTC__##type_name = {0};
#   include "ct_types.def.h"

#define CTI_MAGIC 0xfafb0102

struct CTInstancePool;

typedef struct CTInstance {
    int magic;
    
    CTTypeInfo *type_info;
    CTRuntimeClass *runtime_class;
    struct CTInstancePool *pool;
    
    uint64_t refcount;
    
    /* This does NOT include the CTInstance size!! */
    uint64_t instance_size;
    
    /* The actual class instance will immediately follow this */
    /* instance_data is type or instanced-defined data that is
     * useful to have around or necessary for proper function.
     * For example, a string type might store its length */
    void *instance_data;
} CTInstance;

#define CT_INSTANCE(obj) ((CTInstance*)((obj) - sizeof(CTInstance)))
#define CT_OBJ(instance) ((void *)(((uint8_t*)instance) + sizeof(CTInstance)))

#pragma mark Instance Pools

typedef struct CTInstancePool {
    struct CTInstancePool *prev;
    struct CTInstancePool *next;
    
    // TODO(bloggins): lock access to these if we have threads
    
    CTInstance *instance;
} CTInstancePool;

static CTInstancePool *global_pool = NULL;

CTInstancePool *ct_pool_create(CTInstance *instance) {
    // TODO(bloggins): create chunks of space for this at a time
    CTInstancePool *pool = (CTInstancePool*)calloc(1, sizeof(CTInstancePool));
    pool->instance = instance;
    pool->next = NULL;
    pool->prev = NULL;
    
    if (instance) {
        assert(!instance->pool);
        instance->pool = pool;
    }
    
    return pool;
}

void ct_pool_insert(CTInstancePool *last, CTInstancePool *pool) {
    assert(last);
    assert(pool);
    assert(!pool->next);
    assert(!pool->prev);
    
    CTInstancePool *first = last->next;
    last->next = pool;
    pool->next = first;
    pool->prev = last;
}

void ct_pool_remove(CTInstance *instance) {
    assert(instance);
    assert(instance->pool);
    
    CTInstancePool *pool = instance->pool;
    
    CTInstancePool *prev = pool->prev;
    CTInstancePool *next = pool->next;
    
    // Slice
    prev->next = next;
    next->prev = prev;
    
    instance->pool = NULL;
    free(pool);
}


#pragma mark Default CTRuntimeClass VTABLE

void default_rtinit(CTTypeInfo *type_info, CTRuntimeClass *runtime_class) {
    assert(type_info);
    assert(runtime_class);
    assert(type_info->runtime_class == runtime_class);
    
    if (runtime_class->rtinit_fn == NULL) {
        runtime_class->rtinit_fn = default_rtinit;
    }
    
    if (runtime_class->alloc_fn == NULL) {
        runtime_class->alloc_fn = default_alloc;
    }
    
    if (runtime_class->dealloc_fn == NULL) {
        runtime_class->dealloc_fn = default_dealloc;
    }
    
    if (runtime_class->init_fn == NULL) {
        runtime_class->init_fn = default_init;
    }
    
    if (runtime_class->retain_fn == NULL) {
        runtime_class->retain_fn = default_retain;
    }
    
    if (runtime_class->release_fn == NULL) {
        runtime_class->release_fn = default_release;
    }
    
    if (runtime_class->dump_fn == NULL) {
        runtime_class->dump_fn = (CTDumpFn)default_dump;
    }
}

void *default_alloc(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, size_t extra_bytes) {
    assert(type_info);
    assert(runtime_class);
    assert(type_info->type_base_size);
    
    size_t instance_size = type_info->type_base_size + runtime_class->extra_size + extra_bytes;
    
    CTInstance *instance = (CTInstance *)calloc(1, sizeof(CTInstance) + instance_size);
    instance->magic = CTI_MAGIC;
    instance->type_info = type_info;
    instance->runtime_class = runtime_class;
    instance->instance_size = instance_size;
    
    // TODO(bloggins): alloc different instance pools?
    if (global_pool == NULL) {
        global_pool = ct_pool_create(NULL); /* null will mark the start of the pool */
        global_pool->next = global_pool;
        global_pool->prev = global_pool;
    }
    
    CTInstancePool *pool = ct_pool_create(instance);
    ct_pool_insert(global_pool->prev, pool);
    
    void *obj = ((uint8_t*)instance + sizeof(CTInstance));
    runtime_class->retain_fn(type_info, runtime_class, obj);
    
    return obj;
}

void default_dealloc(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
    assert(inst->magic == CTI_MAGIC);
    assert(inst->refcount == 0);
    
    if (inst->pool) {
        ct_pool_remove(inst);
    }
    
    free(inst);
}

void default_init(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
    assert(inst->magic == CTI_MAGIC);
    assert(inst->refcount > 0);
    assert(inst->instance_size > 0);
    
    // NOTE(bloggins): don't do anything with obj here, because we
    // never know what type of object it is
}

void default_retain(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
    assert(inst->magic == CTI_MAGIC);
    assert(inst->instance_size > 0);
    
    ++inst->refcount;
    assert(inst->refcount > 0);
}

void default_release(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
    assert(inst->magic == CTI_MAGIC);
    assert(inst->refcount > 0);
    assert(inst->instance_size > 0);
    
    --inst->refcount;
    if (inst->refcount == 0) {
        runtime_class->dealloc_fn(type_info, runtime_class, obj);
    }
}

void default_dump(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, FILE *f, void *obj) {
    fprintf(f, "<%s 0x%x (refcount: %llu)>", type_info->type_name,
            (unsigned int)obj, CT_INSTANCE(obj)->refcount);
}

#pragma Overridable Functions

// TODO(bloggins): Have macros that defined the default implementation of
// <type>_dump, etc. These will have __attribute__((weak)) and just call default_
//
// If a type defines it as non-weak, then it will override this implementation

/*
int __attribute__((weak)) MyFoo() {
    return 3;
}
*/

//#define CT_TYPE(type_id, supertype_id, type_name)                               \
//#   include "ct_types.def.h"

#pragma Public API

void ct_init(void) {
    //fprintf(stderr, "MYFOO = %d\n", MyFoo());
    for (int i = 0; i < CT_TYPE_CTLast; i++) {
        CTTypeInfo *type_info = &CT_TYPE_INFO[i];
        assert(type_info);
        assert(type_info->type_id == i);
        
        type_info->type_base_size = CT_BASE_SIZES[i];
        assert(type_info->type_base_size > 0);
        
        type_info->runtime_class = CT_RUNTIME_CLASS[i];
        
        CTRuntimeClass *runtime_class = type_info->runtime_class;
        assert(runtime_class);
        
        if (runtime_class->rtinit_fn == NULL) {
            runtime_class->rtinit_fn = default_rtinit;
        }
        
        runtime_class->rtinit_fn(type_info, runtime_class);
    }
}

void *ct_create(CTTypeID type, size_t extra_bytes) {
    CTTypeInfo *type_info = &CT_TYPE_INFO[type];
    
    return type_info->runtime_class->alloc_fn(type_info, type_info->runtime_class, extra_bytes);
}

void ct_retain(void *obj) {
    assert(obj);
    
    CTInstance *instance = CT_INSTANCE(obj);
    assert(instance);
    assert(instance->magic = CTI_MAGIC);
    assert(instance->runtime_class);
    assert(instance->runtime_class->retain_fn);
    
    instance->runtime_class->retain_fn(instance->type_info, instance->runtime_class, obj);
}

void ct_release(void *obj) {
    assert(obj);
    
    CTInstance *instance = CT_INSTANCE(obj);
    assert(instance);
    assert(instance->magic = CTI_MAGIC);
    assert(instance->runtime_class);
    assert(instance->runtime_class->release_fn);
    
    instance->runtime_class->release_fn(instance->type_info, instance->runtime_class, obj);
}

void ct_autorelease() {    
    CTInstancePool *pool = global_pool;
    assert(pool);
    assert(!pool->instance);    // null instance is how we signal the start
    pool = pool->next;
    while (true) {
        if (!pool->instance) {
            break;
        }
        
        CTInstance *instance = pool->instance;
        if (instance->refcount > 1) {
            fprintf(stderr, "[over-retain] <%s 0x%x> refcount: %llu\n",
                    instance->type_info->type_name, (unsigned int)instance, instance->refcount);
        }
        
        CTInstancePool *next = pool->next;
        instance->runtime_class->release_fn(instance->type_info, instance->runtime_class, CT_OBJ(instance));
        
        pool = next;
    }
}

void ct_dump(void *obj) {
    assert(obj);
    
    CTInstance *instance = CT_INSTANCE(obj);
    assert(instance->magic == CTI_MAGIC);
    
    assert(instance->runtime_class);
    assert(instance->runtime_class->dump_fn);
    instance->runtime_class->dump_fn(instance->type_info, instance->runtime_class, stderr, obj);
}


