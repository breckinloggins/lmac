//
//  ct.h
//  lmac
//
//  Created by Breckin Loggins on 12/13/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// The compiler's "compile time" library

#ifndef lmac_ct_h
#define lmac_ct_h

#include <stdint.h>

/* The "none" type or "invalid" type, used to catch errors */
#define CT_TYPE_ID_INVALID  0x00

/* The "reserved" type, perhaps used in the future to allow more than 254
 * compile time types */
#define CT_TYPE_ID_RESERVED 0xFF

#define CT_TYPE_ID(type_name) CT_TYPE_##type_name

typedef enum {
#   define CT_TYPE(type_name, ...)    CT_TYPE_ID(type_name),
#   include "ct_types.def.h"
} CTTypeID;

struct CTRuntimeClass;

#define CT_TYPE(type_name) extern struct CTRuntimeClass _RTC__##type_name;
#   include "ct_types.def.h"

typedef void CTNone;
typedef void CTLast;
struct CTTypeInfo;

/*                          */
/* CTRuntimeClass VTABLE    */
/*                          */

/* called once by ct_init for each CTRuntimeClass */
typedef void (*CTRTInitFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class);

/* called when a new CT object is needed (default allocator is calloc(3) */
typedef void * (*CTAllocFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class, size_t extra_bytes);

/* called when a CT object needs to be deallocated */
typedef void (*CTDeallocFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class, void *obj);

/* called when a CT object is (re)initialized */
typedef void (*CTInitFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class, void *obj);

typedef void (*CTRetainFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class, void *obj);

typedef void (*CTReleaseFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class, void *obj);

/* file_handle is normally a FILE*  */
typedef void (*CTDumpFn)(struct CTTypeInfo *type_info, struct CTRuntimeClass *runtime_class, void *file_handle, void *obj);

/*                          */
/* End VTABLE               */
/*                          */

typedef struct CTRuntimeClass {
    /* must be first */
    int32_t magic;
    
    /* more bytes to be allocated in addition to the CT type's known size */
    size_t extra_size;
    
    /* vtable */
    CTRTInitFn rtinit_fn;
    CTAllocFn alloc_fn;
    CTDeallocFn dealloc_fn;
    CTInitFn init_fn;
    CTRetainFn retain_fn;
    CTReleaseFn release_fn;
    CTDumpFn dump_fn;
} CTRuntimeClass;

typedef struct CTTypeInfo {
    uint64_t type_id;
    
    const char *type_name;
    size_t type_base_size;
    
    CTRuntimeClass *runtime_class;
} CTTypeInfo;

//extern int MyFoo();

extern size_t CT_BASE_SIZES[CT_TYPE_ID_RESERVED];
extern CTTypeInfo CT_TYPE_INFO[CT_TYPE_ID_RESERVED];
extern CTRuntimeClass *CT_RUNTIME_CLASS[CT_TYPE_ID_RESERVED];

#ifndef CT_INIT_IMPLEMENTATION
__attribute__((constructor(1)))
static void __ct_initialize_base_sizes() {
    #   define CT_TYPE(type_name) \
    CT_BASE_SIZES[CT_TYPE_ID(type_name)] = sizeof(type_name);
    #include "ct_types.def.h"
}
#endif

#pragma mark Public API

void *ct_create(CTTypeID type, size_t extra_bytes);
void ct_retain(void *obj);
void ct_release(void *obj);
void ct_autorelease(void /* multiple pools in the future? */);
void ct_dump(void *obj);
#endif
