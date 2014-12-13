//
//  diag.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/3/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef DIAG_KIND
#define DIAG_KIND(kind, name)
#endif

DIAG_KIND(INFO, info)
DIAG_KIND(WARNING, warning)
DIAG_KIND(ERROR, error)

/* Used for internal compiler errors */
DIAG_KIND(FATAL, fatal)

/* Should ALWAYS be last */
DIAG_KIND(LAST, last)

#undef DIAG_KIND

#ifndef DIAG
#define DIAG(kind, name)
#endif
