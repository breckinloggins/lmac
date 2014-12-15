//
//  ct_api.h
//  lmac
//
//  Created by Breckin Loggins on 12/15/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// API for CT system. Include the full "ct.h" instead if you want to actually implement
// the CT Types (it will include this file).

#ifndef lmac_ct_api_h
#define lmac_ct_api_h

#include <stdint.h>

typedef uint64_t CTTypeID;
typedef void (*CTTypeInit)(CTTypeID type_id);
#define CT_TYPE_ID(type_name) CT_TYPE_##type_name

void *ct_create(CTTypeID type, size_t extra_bytes);
void ct_retain(void *obj);
void ct_release(void *obj);
void ct_autorelease(void /* multiple pools in the future? */);
void ct_dump(void *obj);

#endif
