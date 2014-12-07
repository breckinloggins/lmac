//
//  ast.c
//  lmac
//
//  Created by Breckin Loggins on 12/2/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// NOTE(bloggins): Take a look at
//                  https://www.cs.utah.edu/flux/flick/current/doc/guts/gutsch6.html
//                 for a relatively clean C/C++ AST

#include "clite.h"

// NOTE(bloggins): We're doing lots of small allocations here. It might
//                  be worth it to use a pool allocator or other optimization
//                  later.

global_variable const char *g_ast_names[] = {
#   define AST(kind, ...) "AST_"#kind ,
#   include "ast.def.h"
};

const char *ast_get_kind_name(ASTKind kind) {
    return g_ast_names[kind];
}


ASTBase *ast_create(ASTKind kind, size_t size) {
    ASTBase *node = (ASTBase *)calloc(1, size);
    node->kind = kind;
    
    return node;
}

//
// Visitor acceptors
//

#define AST_ACCEPT_FN_NAME(kind) accept_##kind
#define AST_ACCEPT_FN(kind) int AST_ACCEPT_FN_NAME(kind)(ASTBase *node, VisitFn visit, void *ctx)

#define STANDARD_VISIT_PRE()                                    \
    int result = visit(node, VISIT_PRE, ctx);                   \
    if (result != VISIT_OK) {                                   \
        return result == VISIT_HANDLED ? VISIT_OK : result;     \
    }

#define STANDARD_VISIT_POST()                                   \
    return visit(node, VISIT_POST, ctx);


#define STANDARD_VISIT()                                        \
    STANDARD_VISIT_PRE()                                        \
    STANDARD_VISIT_POST()

#define STANDARD_ACCEPT(node)                                               \
    if ((node) != NULL) {                                                   \
        result = ((ASTBase*)(node))->accept(((ASTBase*)(node)), visit, ctx);\
        if (result != VISIT_OK) {                                           \
            return result;                                                  \
        }                                                                   \
    }

AST_ACCEPT_FN(AST_UNKNOWN) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_BASE) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_LAST) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_IDENT) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_OPERATOR) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_EXPR_EMPTY) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_EXPR_IDENT) {
    STANDARD_VISIT_PRE()
    
    ASTExprIdent *ident = (ASTExprIdent*)node;
    STANDARD_ACCEPT(ident->name)
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_EXPR_NUMBER) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_EXPR_STRING) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_EXPR_CAST) {
    STANDARD_VISIT_PRE()
    
    ASTExprCast *cst = (ASTExprCast *)node;
    STANDARD_ACCEPT(cst->type)
    STANDARD_ACCEPT(cst->expr)
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_EXPR_PAREN) {
    STANDARD_VISIT_PRE()
    
    ASTExprParen *paren = (ASTExprParen*)node;
    STANDARD_ACCEPT(paren->inner);
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_EXPR_BINARY) {
    STANDARD_VISIT_PRE()
    ASTExprBinary *binop = (ASTExprBinary*)node;
    
    STANDARD_ACCEPT(binop->left);
    STANDARD_ACCEPT(binop->op);
    STANDARD_ACCEPT(binop->right);
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_EXPR_CALL) {
    STANDARD_VISIT_PRE()
    
    ASTExprCall *call = (ASTExprCall*)node;
    STANDARD_ACCEPT(call->callable);
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_DEFN_FUNC) {
    STANDARD_VISIT_PRE()
    ASTDefnFunc *defn = (ASTDefnFunc*)node;
    
    STANDARD_ACCEPT(defn->type)
    STANDARD_ACCEPT(defn->base.name)
    STANDARD_ACCEPT(defn->block)
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_DEFN_VAR) {
    STANDARD_VISIT_PRE()
    ASTDefnVar *defn = (ASTDefnVar*)node;
    
    STANDARD_ACCEPT(defn->type)
    STANDARD_ACCEPT(defn->base.name)
    STANDARD_ACCEPT(defn->expression)
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_BLOCK) {
    STANDARD_VISIT_PRE()
    
    ASTBlock *b = (ASTBlock*)node;
    
    List_FOREACH(ASTBase*, defn, b->statements, {
        STANDARD_ACCEPT(defn)
    })
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_STMT_RETURN) {
    STANDARD_VISIT_PRE()
    
    STANDARD_ACCEPT(((ASTStmtReturn*)node)->expression);
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_STMT_EXPR) {
    STANDARD_VISIT_PRE()
    
    STANDARD_ACCEPT(((ASTStmtExpr*)node)->expression);
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_TOPLEVEL) {
    STANDARD_VISIT_PRE()
    
    ASTTopLevel *tl = (ASTTopLevel*)node;

    List_FOREACH(ASTBase*, defn, tl->definitions, {
        STANDARD_ACCEPT(defn)
    })
    
    STANDARD_VISIT_POST()
}

//
// Preprocessor Visitor
//

AST_ACCEPT_FN(AST_PP_PRAGMA) {
    STANDARD_VISIT_PRE()
    
    // NOTE(bloggins): We don't visit the arg here because the
    // pragma-visitor must decide how to interpret it
    
    STANDARD_VISIT_POST()
}

//
// Type Expression Visitor
//

AST_ACCEPT_FN(AST_TYPE_BEGIN) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_TYPE_CONSTANT) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_TYPE_NAME) {
    STANDARD_VISIT()
}

AST_ACCEPT_FN(AST_TYPE_POINTER) {
    STANDARD_VISIT_PRE()
    
    STANDARD_ACCEPT(((ASTTypePointer*)node)->pointer_to);
    
    STANDARD_VISIT_POST()
}

AST_ACCEPT_FN(AST_TYPE_END) {
    STANDARD_VISIT()
}


//
// Creation and general
//

#define AST(kind, name, type)                               \
type *ast_create_##name() {                                 \
    type *node = (type *)ast_create(AST_##kind,             \
                                    sizeof(ASTBase) +       \
                                    sizeof(type));          \
    ((ASTBase*)node)->accept = accept_AST_##kind;           \
    return node;                                            \
}
#include "ast.def.h"

int ast_visit(ASTBase *node, VisitFn visitor, void *ctx) {
    return node->accept(node, visitor, ctx);
}

int visitor_clean(ASTBase *node, VisitPhase phase, void *ctx) {
    if (phase == VISIT_PRE) {
        free(node->visit_data);
        node->visit_data = NULL;
    }
    
    return VISIT_OK;
}

void ast_visit_data_clean(ASTBase *node) {
    ast_visit(node, visitor_clean, NULL);
}

void ast_fprint(FILE *f, ASTBase *node, int indent_level) {
    char indent[indent_level + 1];
    for (int i = 0; i < indent_level; i++) {
        indent[i] = '\t';
    }
    indent[indent_level] = 0;
    
    fprintf(f, "%s%d:%s", indent, node->location.line, ast_get_kind_name(node->kind));
    if (node->kind == AST_DEFN_VAR || node->kind == AST_IDENT) {
        size_t content_size = node->location.range_end - node->location.range_start;
        char content[content_size + 1];
        char *pc = (char *)node->location.range_start;
        for (int i = 0; i < content_size; i++) {
            content[i] = *pc++;
            if (content[i] == '\n') content[i] = '$';
            if (content[i] == '\t') content[i] = '$';
        }
        content[content_size] = 0;
        
        fprintf(f, " <%s>", content);
    } else if (node->kind == AST_EXPR_NUMBER) {
        fprintf(f, " <%d>", ((ASTExprNumber*)node)->number);
    } else if (node->kind == AST_EXPR_BINARY) {
        ASTExprBinary *binop = (ASTExprBinary*)node;
        fprintf(f, " <%s>", spelling_cstring(binop->op->base.location.spelling));
    }
}

Scope *ast_nearest_scope(ASTBase *node) {
    assert(node && "node should not be null");
    assert(node->location.ctx && "node scope not searchable without parse context");
    
    if (node->scope != NULL) {
        // That's convenient
        return node->scope;
    }
    
    ASTBase *search_node = node;
    while (search_node != NULL) {
        if (search_node->scope != NULL) {
            node->scope = search_node->scope;
            return node->scope;
        }
        
        search_node = search_node->parent;
    }
    
    Context *ctx = node->location.ctx;
    assert(ctx->active_scope && "context does not have an active scope");
    
    // Might as well cache it now. It's not like it's going to change
    node->scope = ctx->active_scope;
    
    return ctx->active_scope;
}

ASTDeclaration* ast_nearest_spelling_definition(Spelling spelling, ASTBase* node) {
    Scope *scope = ast_nearest_scope(node);
    
    do {
        List_FOREACH(ASTBase*, scope_node, scope->declarations, {
            ASTIdent *defn_ident = NULL;
            if (scope_node->kind == AST_DEFN_FUNC) {
                defn_ident = ((ASTDefnFunc*)scope_node)->base.name;
            } else if (scope_node->kind == AST_DEFN_VAR) {
                defn_ident = ((ASTDefnVar*)scope_node)->base.name;
            } else {
                continue;
            }
            
            assert(defn_ident && "definitions should always be named");
            if (spelling_equal(spelling, defn_ident->base.location.spelling)) {
                return (ASTDeclaration*)scope_node;
            }
        })
        
        scope = scope->parent;
    } while (scope != NULL);
    
    return NULL;
}


// TODO(bloggins): Probably need to move into a separate type checking file

bool ast_node_is_type_expression(ASTBase *node) {
    return node != NULL && node->kind > AST_TYPE_BEGIN && node->kind < AST_TYPE_END;
}

bool ast_node_is_type_definition(ASTBase *node) {
    if (node->kind != AST_DEFN_VAR) {
        return false;
    }
    
    ASTDefnVar *var = (ASTDefnVar*)node;
    return (var->type != NULL && (var->type->type_id & TYPE_FLAG_KIND));
}

ASTDeclaration* ast_ident_find_declaration(ASTIdent *ident) {
    if (ident->declaration != NULL) {
        return ident->declaration;
    }
    
    Spelling sp = AST_BASE(ident)->location.spelling;
    ident->declaration = ast_nearest_spelling_definition(sp, (ASTBase*)ident);
    
    return ident->declaration; // May still be null
}

bool ast_ident_is_type_name(ASTIdent *name) {
    ASTDeclaration *type_decl = ast_ident_find_declaration(name);
    if (type_decl == NULL) {
        diag_printf(ERR_ANALYZE, &name->base.location,
                    "I don't know what '%s' is",
                    spelling_cstring(name->base.location.spelling));
        exit(ERR_ANALYZE);
    }
    return ast_node_is_type_definition((ASTBase*)type_decl);
    
}

ASTTypeExpression *ast_typename_resolve(ASTTypeName *name) {
    if (name->resolved_type != NULL) {
        return name->resolved_type;
    }
    
    ASTDeclaration *type_decl = ast_ident_find_declaration(name->name);
    if (type_decl == NULL) {
        return NULL;
    } else if (ast_node_is_type_definition((ASTBase*)type_decl)) {
        // TODO(bloggins): HACK: We need to abstract this to just
        // getting the type of any AST node, because right now this
        // has incestuous knowledge of the implementation of
        // ast_node_is_type_definition!
        ASTDefnVar *defn = (ASTDefnVar*)type_decl;
        assert(defn->expression && "claimed to resolve a type but didn't");
        
        // Is there any reason to NOT cache it?
        name->resolved_type = (ASTTypeExpression*)defn->expression;
        return (ASTTypeExpression*)defn->expression;
    }
    
    return NULL;
}


ASTTypeExpression *ast_type_get_canonical_type(ASTTypeExpression *type) {
    // NOTE(bloggins): Get the "standard" type for this type. For example, the
    // canonical type of a type name is the canonical type of the resolved type
    // name.
    if (type == NULL) {
        // For convenience, the canonical type of NULL is NULL
        return NULL;
    }
    
    switch (AST_BASE(type)->kind) {
        case AST_TYPE_CONSTANT: {
            /* itself */
            return type;
        } break;
        case AST_TYPE_NAME: {
            /* the canonical type of the resolved name if it's resolved */
            ASTTypeName *type_name = (ASTTypeName*)type;
            ast_typename_resolve(type_name);
            if (type_name->resolved_type == NULL) {
                Spelling sp = AST_BASE(type_name)->location.spelling;
                ASTDeclaration *type_decl = ast_nearest_spelling_definition(sp, (ASTBase*)type_name);
                if (type_decl == NULL) {
                    diag_printf(ERR_ANALYZE, &AST_BASE(type_name)->location,
                                "undefined type '%s'",
                                spelling_cstring(sp));
                    exit(ERR_ANALYZE);
                } else {
                    diag_printf(ERR_ANALYZE, &AST_BASE(type_name)->location,
                               "'%s' is a %s, not a type",
                                spelling_cstring(sp),
                                ast_get_kind_name(AST_BASE(type_decl)->kind));
                    exit(ERR_ANALYZE);
                }
            }
            
            return ast_type_get_canonical_type(type_name->resolved_type);
        }
        case AST_TYPE_POINTER: {
            /* NOTE(bloggins): hmmmm, what to do here? At this point I'd just prefer to
             * return itself and see if that works. Just make sure it's resolved first */
            ast_type_get_canonical_type(((ASTTypePointer*)type)->pointer_to);
            return type;
        }
        default: assert(false && "Unhandled type kind");
    }
}

uint32_t ast_type_next_type_id() {
    // TODO(bloggins): Common types should probably be pre-reserved for more stable
    // type ids
    static uint32_t next_id = 1;
    
    return next_id++;
}

void ast_dump(ASTBase *node) {
    ast_fprint(stderr, node, 0);
}

#pragma mark Convenience Initializers

void ast_init_expr_binary(ASTExprBinary *binop, ASTExpression *left,
                          ASTExpression *right, ASTOperator *op) {
    
    AST_BASE(left)->parent = AST_BASE(right)->parent = (ASTBase*)binop;
    
    binop->left = left;
    binop->right = right;
    binop->op = op;
    binop->op->base.parent = (ASTBase*)binop;
}
