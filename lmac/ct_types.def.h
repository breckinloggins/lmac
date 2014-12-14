//
//  ct_types.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/13/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// These are the major object types used in the compiler. This is a
// runtime class-style database used for things like debug dumping,
// pretty printing, lifetime management, and so forth

#ifndef CT_TYPE
#define CT_TYPE(type_constant, super_type, type_name)
#endif

//      Type Constant       Super Type      TypeName
CT_TYPE(CT_NONE,            CT_NONE,        CTNone)
CT_TYPE(CT_AST_BASE,        CT_NONE,        ASTBase)
// TODO(bloggins): other AST types
CT_TYPE(CT_CONTEXT,         CT_NONE,        Context)

/* must be last */
CT_TYPE(CT_LAST,            CT_NONE,        CTLast)
#undef CT_TYPE
