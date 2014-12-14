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

size_t CT_BASE_SIZES[0xFF] = {};
CTRuntimeClass *CT_RUNTIME_CLASS[0xFF] = {};

CTTypeInfo CT_TYPE_INFO[] = {
#   define CT_TYPE(type_id, supertype_id, type_name, runtime_class)     \
    { type_id, #type_id, supertype_id, #supertype_id, #type_name, 0, NULL },
#   include "ct_types.def.h"
};

CTRuntimeClass RTCInvalid = { CT_MAGIC | CT_TYPE_ID_INVALID, 0, NULL };

#define CTI_MAGIC 0xfafb0102

typedef struct CTInstance {
    int magic;
    
    CTTypeInfo *type_info;
    CTRuntimeClass *runtime_class;
    
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
    instance->type_info = type_info;
    instance->runtime_class = runtime_class;
    instance->instance_size = instance_size;
    
    void *obj = (instance + sizeof(CTInstance));
    runtime_class->retain_fn(type_info, runtime_class, obj);
    
    return obj;
}

void default_dealloc(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
    assert(inst->refcount == 0);
    
    free(inst);
}

void default_init(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
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
    assert(inst->instance_size > 0);
    
    ++inst->refcount;
    assert(inst->refcount > 0);
}

void default_release(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, void *obj) {
    assert(type_info);
    assert(runtime_class);
    assert(obj);
    
    CTInstance *inst = CT_INSTANCE(obj);
    assert(inst->refcount > 0);
    assert(inst->instance_size > 0);
    
    --inst->refcount;
    if (inst->refcount == 0) {
        runtime_class->dealloc_fn(type_info, runtime_class, obj);
    }
}

void default_dump(CTTypeInfo *type_info, CTRuntimeClass *runtime_class, FILE *f, void *obj) {
    fprintf(f, "<%s 0x%x>", type_info->type_name, (unsigned int)obj);
}

#pragma Public API

void ct_init(void) {
    for (int i = 0; i < CT_LAST; i++) {
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


