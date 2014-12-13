//
//  type.h
//  lmac
//
//  Created by Breckin Loggins on 12/12/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_type_h
#define lmac_type_h

struct ASTTypeExpression;
struct ASTBase;

struct ASTTypeExpression *type_infer(ASTBase *node);

#endif
