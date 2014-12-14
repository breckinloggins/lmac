//
//  ast.h
//  lmac
//
//  Created by Breckin Loggins on 12/5/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

#ifndef lmac_ast_h
#define lmac_ast_h

extern struct CTRuntimeClass RTC_ASTBase;

typedef enum {
#   define AST(kind, ...) AST_##kind,
#   include "ast.def.h"
} ASTKind;

struct Scope;
struct ASTBase;

typedef enum {
    VISIT_PRE,
    VISIT_POST
} VisitPhase;

#define VISIT_OK        0
#define VISIT_HANDLED   1   /* When returned VISIT_PRE phase, stops default visit and post */
typedef int (*VisitFn)(struct ASTBase *node, VisitPhase phase, void *ctx);

#define AST_BASE(node) ((ASTBase*)(node))
#define AST_IS(node, node_kind) (AST_BASE((node))->kind == (node_kind))

struct ASTTypeExpression;

typedef struct ASTBase {
    /* Leave this as the first member. It will be set to an invalid memory area
     so any attempts to read or write to it will fail */
    int magic;
    
    ASTKind kind;
    SourceLocation location;
    struct ASTBase *parent;
    
    /* Either set in parse or calculated on demand */
    struct Scope *scope;
    
    /* Computed */
    struct ASTTypeExpression *inferred_type;
    
    /* Arbitrary data that visitors can attach to the node. All visit data
     * must be cleaned with ast_visit_data_clean() after each
     * complete logical pass.
     *
     * All data attached to the node must have been created by the system 
     * allocator (usually through malloc/calloc), because ast_visit_data_clean
     * calls free() on the data before setting the pointer to NULL.
     */
    void *visit_data;
    
    /* The visitor function for this node */
    int (*accept)(struct ASTBase *node, VisitFn visitor, void *ctx);
} ASTBase;

struct ASTDeclaration;
struct ASTBlock;

typedef struct ASTIdent {
    ASTBase base;
    
    /* Computed */
    struct ASTDeclaration *declaration;
} ASTIdent;

typedef struct {
    ASTBase base;
    
    Token op;
} ASTOperator;

#pragma mark Expressions AST

typedef struct {
    ASTBase base;
} ASTExpression;

typedef struct {
    ASTExpression base;
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
    
    // string literal. Actual string is the node spelling
} ASTExprString;

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
    List *args;
    
} ASTExprCall;

#pragma mark Declarations AST

typedef struct ASTDeclaration {
    ASTBase base;
    
    struct ASTIdent *name;
} ASTDeclaration;


typedef struct {
    ASTDeclaration base;
    
    struct ASTTypeExpression *type;
    
    List *params;
    bool has_varargs;
    
    struct ASTBlock *block;
    
} ASTDeclFunc;

typedef struct {
    ASTDeclaration base;
    
    struct ASTTypeExpression *type;
    bool is_const;
    
    ASTExpression *expression;
    
} ASTDeclVar;

#pragma mark Statements AST

typedef struct {
    ASTBase base;
} ASTStatement;

typedef struct ASTBlock {
    ASTStatement base;
    
    List *statements;
} ASTBlock;

typedef struct {
    ASTStatement base;
    
    ASTExpression *expression;
} ASTStmtReturn;

typedef struct {
    ASTStatement base;
    
    ASTExpression *expression;
} ASTStmtExpr;

typedef struct {
    ASTStatement base;
    
    ASTDeclaration *declaration;
} ASTStmtDecl;

typedef struct {
    ASTStatement base;
    
    ASTExpression *condition;
    ASTBase *stmt_true;
    ASTBase *stmt_false;
} ASTStmtIf;

typedef struct {
    ASTStatement base;
    
    Token keyword;
    ASTIdent *label;
} ASTStmtJump;

typedef struct {
    ASTStatement base;
    
    ASTIdent *label;
    ASTStatement *stmt;
} ASTStmtLabeled;

#pragma mark Misc AST

typedef struct {
    ASTBase base;
    
    List *definitions;
} ASTTopLevel;

// see http://jhnet.co.uk/articles/cpp_magic for fun
typedef struct {
    ASTBase base;
    
} ASTPPDirective;

typedef struct {
    ASTPPDirective base;
    
    /* Only very simple pragmas of the form
     * #pragma CLITE arg [optional stuff]
     * are supported right now
     */
    ASTIdent *arg;
    Spelling rest;
} ASTPPPragma;

typedef struct {
    ASTPPDirective base;
    
    ASTIdent *name;
    Spelling value;
} ASTPPDefinition;

typedef enum {
    PP_IF_UNKNOWN,
    PP_IF_IFNDEF,
} PPIfKind;

typedef struct {
    ASTPPDirective base;
    
    PPIfKind kind;
} ASTPPIf;

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
#define BIT_FLAG_CHAR           0x02
#define BIT_FLAG_INT            0x04
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
    ASTTypeExpression *resolved_type;
    
} ASTTypeName;

typedef struct {
    ASTTypeExpression base;
    
    ASTTypeExpression *pointer_to;
    
} ASTTypePointer;


const char *ast_get_kind_name(ASTKind kind);
int ast_visit(ASTBase *node, VisitFn visitor, void *ctx);
void ast_visit_data_clean(ASTBase *node);
struct Scope *ast_nearest_scope(ASTBase *node);
ASTDeclaration* ast_nearest_spelling_definition(Spelling spelling, ASTBase* node);
bool ast_node_is_expression(ASTBase *node);
bool ast_node_is_type_definition(ASTBase *node);
bool ast_node_is_type_expression(ASTBase *node);
ASTDeclaration* ast_ident_find_declaration(ASTIdent *ident);
ASTBase *ast_ident_find_label(ASTIdent *name);
bool ast_ident_is_type_name(ASTIdent *name);
ASTTypeExpression *ast_typename_resolve(ASTTypeName *name);
ASTTypeExpression *ast_type_get_canonical_type(ASTTypeExpression *type);
uint32_t ast_type_next_type_id();
void ast_init_expr_binary(ASTExprBinary *binop, ASTExpression *left,
                          ASTExpression *right, ASTOperator *op);
void ast_dump(ASTBase *node);

void ast_fprint(FILE *f, ASTBase *node, int indent);

#endif
