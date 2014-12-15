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
#define CT_TYPE(type_name)
#endif

//      Type Constant       Super Type      TypeName
CT_TYPE(List)
CT_TYPE(ASTBase)
// TODO(bloggins): other AST types
CT_TYPE(Context)
CT_TYPE(Scope)

/* must be last */
CT_TYPE(CTLast)

#undef CT_TYPE

