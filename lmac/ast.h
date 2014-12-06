//
//  ast.h
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_ast_h
#define lmac_ast_h

typedef enum {
#   define AST(kind, ...) AST_##kind,
#   include "ast.def.h"
} ASTKind;


struct ASTBase;
struct ASTList;
typedef struct ASTList {
    struct ASTBase *node;
    struct ASTList *next;
} ASTList;

#define ASTLIST_FOREACH(type, var_name, ast_list, block)                    \
do {                                                                        \
ASTList *list_copy = ast_list;                                          \
while((list_copy) != NULL) {                                            \
type var_name = (type)(list_copy)->node;                            \
assert(var_name != NULL);                                           \
list_copy = list_copy->next;                                        \
block                                                               \
}                                                                       \
} while(0);

typedef enum {
    VISIT_PRE,
    VISIT_POST
} VisitPhase;

#define VISIT_OK        0
#define VISIT_HANDLED   1   /* When returned VISIT_PRE phase, stops default visit and post */
typedef int (*VisitFn)(struct ASTBase *node, VisitPhase phase, void *ctx);

#define AST_BASE(node) ((ASTBase*)(node))

struct ASTTypeExpression;

typedef struct ASTBase {
    ASTKind kind;
    SourceLocation location;
    struct ASTBase *parent;
    
    int (*accept)(struct ASTBase *node, VisitFn visitor, void *ctx);
} ASTBase;

typedef struct {
    ASTBase base;
    
    ASTList *statements;
} ASTBlock;

typedef struct {
    ASTBase base;
} ASTIdent;

typedef struct {
    ASTBase base;
    
    Token op;
} ASTOperator;

typedef struct {
    ASTBase base;
    
    struct ASTTypeExpression *type;
    ASTIdent *name;
    ASTBlock *block;
    
} ASTDefnFunc;

#pragma mark Expressions AST

typedef struct {
    ASTBase base;
    
    // TODO(bloggins): store inferred type
} ASTExpression;

typedef struct {
    ASTBase base;
} ASTExprEmpty;

typedef struct {
    ASTExpression base;
    
    ASTIdent *name;
} ASTExprIdent;

typedef struct {
    ASTExpression base;
    
    int number;
} ASTExprNumber;

typedef struct {
    ASTExpression base;
    
    ASTExpression *inner;
} ASTExprParen;

typedef struct {
    ASTExpression base;
    
    struct ASTTypeExpression *type;
    ASTExpression *expr;
} ASTExprCast;

typedef struct {
    ASTExpression base;
    
    ASTExpression *left;
    ASTOperator *op;
    ASTExpression *right;
} ASTExprBinary;

typedef struct {
    ASTExpression base;
    
    ASTExpression *callable;
    
} ASTExprCall;

#pragma mark Statements AST

typedef struct {
    ASTBase base;
    
    ASTExpression *expression;
} ASTStmtReturn;

typedef struct {
    ASTBase base;
    
    ASTExpression *expression;
} ASTStmtExpr;

#pragma mark Mist AST

typedef struct {
    ASTBase base;
    
    struct ASTTypeExpression *type;
    ASTIdent *name;
    
    ASTExpression *expression;
    
} ASTDefnVar;

typedef struct {
    ASTBase base;
    
    ASTList *definitions;
} ASTTopLevel;

// see http://jhnet.co.uk/articles/cpp_magic for fun
typedef struct {
    ASTBase base;
    
} ASTPPDirective;

typedef struct {
    ASTPPDirective base;
    
    /* Only very simple pragmas of the form
     * #pragma CLITE arg
     * are supported right now
     */
    ASTIdent *arg;
} ASTPPPragma;

/*
 * Type Expression AST
 */

#define TYPE_FLAG_UNIT          0
#define TYPE_FLAG_SINGLETON     0x40000000
#define TYPE_FLAG_KIND          0x80000000

typedef struct ASTTypeExpression {
    ASTExpression base;
    
    uint32_t type_id;
} ASTTypeExpression;

#define BIT_FLAG_NONE           0
#define BIT_FLAG_FP             0x01
#define BIT_FLAG_SIGNED         0x80

typedef struct {
    ASTTypeExpression base;
    
    /* Signed, float, etc. */
    uint8_t bit_flags;
    
    // BLOGGINS(TODO): Needs to be machine word size instead of explicitly
    // 64-bits. Why might you ever want a type that's as big as one bit per
    // byte of available memory? It can then model the entire memory space
    // of a system as a bitfield. Granted it would have to be sparse, but still.
    uint64_t bit_size;
    
} ASTTypeConstant;

typedef struct {
    ASTTypeExpression base;
    
    /* The identifier that might or might not be a type... possible... one fine day */
    ASTIdent *name;
    
    /* Computed */
    struct {
        ASTTypeExpression *resolved_type;
    };
    
} ASTTypeName;


const char *ast_get_kind_name(ASTKind kind);
int ast_visit(ASTBase *node, VisitFn visitor, void *ctx);
void ast_list_add(ASTList **list, ASTBase *node);
void ast_fprint(FILE *f, ASTBase *node, int indent);
ASTBase *ast_nearest_scope_node(ASTBase *node);
ASTBase* ast_nearest_spelling_definition(Spelling spelling, ASTBase* node);
bool ast_node_is_type_definition(ASTBase *node);
bool ast_node_is_type_expression(ASTBase *node);
ASTTypeExpression *ast_type_get_canonical_type(ASTTypeExpression *type);
uint32_t ast_type_next_type_id();
void ast_init_expr_binary(ASTExprBinary *binop, ASTExpression *left,
                          ASTExpression *right, ASTOperator *op);

#endif
