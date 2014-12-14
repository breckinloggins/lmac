//
//  ct_init.c
//  lmac
//
//  Created by Breckin Loggins on 12/13/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// #include this file in a function somewhere to initialize the compile-time system

#   define CT_TYPE(type_id, supertype_id, type_name) \
    CT_BASE_SIZES[type_id] = sizeof(type_name); \
    CT_RUNTIME_CLASS[type_id] = &(_RTC__##type_name);
#include "ct_types.def.h"

ct_init();

