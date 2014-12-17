//
//  interp.c
//  lmac
//
//  Created by Breckin Loggins on 12/9/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// Compile-time Code Interpreter (CI)

// FR Byte Code Format (FR = "Front-end Representation")

// Fun research links:
// - https://www.webkit.org/blog/189/announcing-squirrelfish/ (which also has a TON of links to introductory reading on interpreters)
// - http://blog.mozilla.org/dmandelin/2008/06/03/squirrelfish/
// - http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables

// Idea:
// - Implement a (possibly direct-threaded) high level bytecode interpreter
// - Eventual just in time compilation and caching of the AST to bytecode
// - The high level bytecode (see http://www.webkit.org/specs/squirrelfish-bytecode.html) for JS example is in terms
//   of compile-type operations rather than run-type operations. An example byte code could be "push scope"

//
// Design Notes
//
// 1. The entire byte code and meta data should be serializable to and from disk
// 2. The byte code data includes the entire source of the program as typed (so string literals, type names, etc can just refer to the original spelling)
// 3. The walk of the AST is encoded into the byte code (AST_PUSH_NODE / AST_POP_NODE)
// 4. The FR Image is designed to allow worldwide unique coding of things like constants and metadata rather than focusing on small size
// 5. The metadata key and constant type IDs should mirror IPv6 addresses such that they can be namespaced and possibly looked up for metadata

//
// FR Byte Code Image Layout
//
// (TBD, this is just a sketch right now and the initial version (0) will be simpler)
//
// uint32_t FR_MAGIC; (= 0xSOMETHING_CLEVER_IN_HEX)
// uint8_t version1; (= 1)
// uint8_t version2; (= 0)
// uint16_t reserved; (= 0)
// uint64_t tables_offset; (=offset into file of data tables)
//
// <... FR byte code >
//
// @tables_offset:
// uint64_t constants_offset;
// uint64_t metadata_offset;
// uint64_t sources_offset;
// uint64_t sources_count; // These decay into constants, do we need a separate table? Probably want a distinct constant type per language (C, Cx, Python, etc.)
// uint64_t freeze_dried_data_offset;   (= data until end of file)
// ...
//
// @constants_offset:
// uint64_t constants_count;
// [array of constants_count of these]
// uint64_t constant_idx;
// uint64_t constant_type1;
// uint64_t constant_type2; (= usually zero) (128 bit constants allows world-wide consistent type GUIDs)
// uint64_t constant_size;
// [uint8_t] constant_data (of constant_size)
//
// @metadata_offset:
// uint64_t metadata_count;
// uint64_t metadata_key1;
// uint64_t metadata_key2; (= usually zero) (128 bit keys allow world-wide metadata key GUIDs)
//
// @sources_offset: [(* sources_count)]
// uint64_t source_name_constant_idx1;  (= lookup into the constant table)
// uint64_t source_name_constant_idx2;
//
// @freeze_dried_data_offset: (= plain ol' data until end of file)
// <EOF>

// CONSTANT TYPES:
// 0: (void)
// 1: integer
// 2: string
// etc.
// Future ones could be like
// 40: URL

// INTERPRETER VALUE:
//
// The end result of an interpreter run is a single CIValue. This could be of
// any kind including an ASTNode or a Value tree
//
// The interpreter pushes ASTNode values on the tree as they are encountered.
// Its goal is to NOT leave ASTNode values on the tree. This can't always happen
// because the environment may not have the appropriate symbols, functions, etc.
// defined.

#include "clite.h"

#define CI_ERROR(sl, ...)                                              \
diag_printf(DIAG_ERROR, sl, __VA_ARGS__);                               \
exit(ERR_INTERPRET);

#pragma mark ByteStream

typedef struct ByteStream {
    uint8_t *data;
    
    size_t length;
    off_t current_offset;
    
} ByteStream;

void stream_append(ByteStream *stream, uint8_t *data, size_t length) {
#   define CHUNK_SIZE 4096
    assert(stream);
    assert(data);
    
    if (stream->current_offset == stream->length) {
        stream->length += CHUNK_SIZE;
        stream->data = realloc(stream->data, stream->length);
    }
    
    for (size_t idx = 0; idx < length; idx++) {
        stream->data[stream->current_offset] = data[idx];
        ++stream->current_offset;
    }
}

#pragma mark CIValue, CIValueArray

typedef enum {
    /* must be the first so 0 maps to void */
    CIV_VOID,
    
    /* index pointer to another value */
    CIV_VALUE_REF,
    
    /* plain ol' data */
    CIV_POD_CHAR,
    CIV_POD_INTEGER,
    CIV_POD_STRING,
    
    /* value literals (not the same as pod values because they could be retinterpreted in the environment) */
    CIV_CHAR_LITERAL,
    CIV_INTEGER_LITERAL,
    CIV_STRING_LITERAL,
    
    /* data structures */
    CIV_ARRAY,
    
    CIV_IDENTIFIER,
    CIV_BINDING,
    CIV_BINOP,
    CIV_CALL,
    
    CIV_AST_NODE,
    
    /* must be the last */
    CIV_LAST,
} CIValueKind;

typedef uint64_t IdentifierIndex;
typedef uint64_t ValueTableIndex;
typedef uint64_t BindingIndex;

typedef struct CIValue {
    CIValueKind kind;
    union {
        struct {
            bool is_pod;
            
            char char_value;
            uint64_t int_value;
            const char *str_value;
        } simple_value;
        
        ValueTableIndex ref_id;
        IdentifierIndex identifier_id;
        
        struct {
            uint64_t constraint_index;
            IdentifierIndex name_index;
            ValueTableIndex value_index;
        } binding_value;
        
        struct {
            ValueTableIndex op_index;
            
            ValueTableIndex lhs_index;
            ValueTableIndex rhs_index;
        } binop_value;
        
        struct {
            ValueTableIndex callable_index;
            ValueTableIndex arglist_index;
        } call_value;
        
        struct {
            ASTKind kind;
            ValueTableIndex source_index;
            uint64_t start;
            uint64_t end;
        } ast_node_value;
    };
} CIValue;

void ci_value_fprint(FILE *f, Context *ctx, CIValue *value) {
    const CIValue *v = value;
    switch (v->kind) {
        case CIV_VOID: break;
        case CIV_CHAR_LITERAL: fprintf(f, "%c", v->simple_value.char_value); break;
        case CIV_INTEGER_LITERAL: fprintf(f, "%llu", v->simple_value.int_value); break;
        case CIV_STRING_LITERAL: fprintf(f, "%s", v->simple_value.str_value); break;
        case CIV_BINDING: {
            fprintf(f, "(constraint: %llu, name: %llu, value: %llu)",
                    v->binding_value.constraint_index,
                    v->binding_value.name_index,
                    v->binding_value.value_index);
        } break;
        case CIV_BINOP: {
            fprintf(f, "(lhs: %llu, op: %llu, rhs: %llu)",
                    v->binop_value.lhs_index,
                    v->binop_value.op_index,
                    v->binop_value.rhs_index);
        } break;
        case CIV_IDENTIFIER: {
            fprintf(f, "id: %llu", v->identifier_id);
        } break;
        case CIV_AST_NODE: {
            fprintf(f, "(ast_kind: %s, source: ", ast_get_kind_name(v->ast_node_value.kind));
            // TODO(bloggins): We need to serialize this in the FR data as a source constant!
            
            int print_count = 0;
            for (uint64_t idx = v->ast_node_value.start; idx < v->ast_node_value.end; idx++) {
                char c = ctx->buf[idx];
                if (c == '\n' || c == '\r') {
                    c = '$';
                }
                
                fprintf(f, "%c", c);
                if (++print_count == 30) {
                    // That's enough to get the gist
                    fprintf(f, " // ...");
                    break;
                }
                
                fprintf(f, ")");
            }
        } break;
        default: assert(false && "unrecognized value kind");
    }
}

typedef struct {
    /* low number so we run into it sooner rather than later and are forced
     * to use a better data structure.
     */
#   define CI_MAX_VALUES    128

    size_t count;
    CIValue values[CI_MAX_VALUES];
} CIValueTable;

ValueTableIndex values_new(CIValueTable *table, CIValue *v) {
    assert(table);
    assert(v);
    assert((table->count < CI_MAX_VALUES) && "time for a new data structure");

    table->values[table->count++] = *v;
    
    return table->count - 1;
}

#pragma mark Opcodes


typedef enum CIOp {
#   define CI_OP(kind) kind,
#   include "ci_opcodes.def.h"
} CIOp;

static const char *g_opcode_names[] = {
#   define CI_OP(kind) #kind ,
#   include "ci_opcodes.def.h"
};

#pragma mark Byte Code Assembler

// TODO(bloggins): This depends on the endianess of the host processor. BAD!
#define ASM_U16(v) { uint16_t temp = (v); stream_append(stream, (uint8_t*)&temp, 2); }
#define ASM_U32(v) { uint32_t temp = (v); stream_append(stream, (uint8_t*)&temp, 4); }
#define ASM_U64(v) { uint64_t temp = (v); stream_append(stream, (uint8_t*)&temp, 8); }

#define ASM_OP(op) { uint8_t temp = (op); stream_append(stream, &temp, 1); }
#define ASM_OP_1U8(op, byte) { uint8_t temp[] = {(op), (byte)}; stream_append(stream, temp, 2); }

static inline void asm_single_op(ByteStream *stream, CIOp op) {
    assert(op != CIO_LAST);
    ASM_OP(op);
}

static inline void asm_push_u64(ByteStream *stream, int64_t value) {
    ASM_OP(CIO_PUSH_U64);
    ASM_U64(value);
}

static inline void asm_push_node(ByteStream *stream, ASTKind kind) {
    assert((uint32_t)kind < 0xFF && "too many node kinds, need to expand opcode encoding for additional nodes after 1 byte length");
    ASM_OP_1U8(CIO_PUSH_NODE, (uint8_t)kind);
}

#pragma mark Byte Code Decoder

static inline uint64_t decode_u64(uint8_t **ip) {
    // TODO(bloggins): relies on endianess of the host processor!
    uint64_t result = *(uint64_t*)*ip;
    *ip += sizeof(uint64_t);
    
    return result;
}

#pragma mark Visitor Overrides
// *** OVERRIDES GO HERE ***

// Defined in interp_default.c
#define AST(kind, name, type)                                                   \
int ci_visit_AST_##kind(type *node, VisitPhase phase, struct ByteStream *stream);
#include "ast.def.h"
#undef AST

#define CI_VISITOR(kind, type) int ci_visit_##kind(type *node, VisitPhase phase, ByteStream *stream)


CI_VISITOR(AST_TOPLEVEL, ASTTopLevel) {
    // TODO(bloggins): This is a hack because we don't yet support nested contexts
    // that resolve to different source files, so all our current SourceLocation offsets
    // MUST map to our top-level context
    static int tl_count = 0;
    
    if (phase == VISIT_PRE) {
        ++tl_count;
        assert(tl_count == 1 && "nested top levels (e.g. from #include) are not yet supported");
    } else {
        --tl_count;
    }
    
    return VISIT_OK;
}

CI_VISITOR(AST_STMT_DECL, ASTStmtDecl) {
    return VISIT_OK;
}

CI_VISITOR(AST_STMT_EXPR, ASTStmtExpr) {
    return VISIT_OK;
}

CI_VISITOR(AST_BLOCK, ASTBlock) {
    if (phase == VISIT_PRE) {
        // Insert a block guard stub so that we jump around the block unless
        // explicitly told to jump to the block entry point
        
        // Will be freed after visit
        AST_BASE(node)->visit_data = calloc(1, sizeof(off_t));
        *(off_t*)(AST_BASE(node)->visit_data) = stream->current_offset + 1; // past the opcode
        
        asm_push_u64(stream, 0);
        asm_single_op(stream, CIO_JUMP);
    } else {
        // Now that we have the location after the block, put that in the
        // jump guard
        off_t guard_ins_ptr = *(off_t*)(AST_BASE(node)->visit_data);
        off_t saved_stream_ptr = stream->current_offset;
        stream->current_offset = guard_ins_ptr;
        asm_push_u64(stream, saved_stream_ptr);
        stream->current_offset = saved_stream_ptr;
    }
    return VISIT_OK;
}

CI_VISITOR(AST_DECL_FUNC, ASTDeclFunc) {
    return VISIT_OK;
}

CI_VISITOR(AST_DECL_VAR, ASTDeclVar) {
    
    if (phase == VISIT_POST) {
        // TODO(bloggins): Tell it what the identifier is!
        asm_single_op(stream, CIO_NEW_IDENTIFIER);
        
        if (node->expression == NULL) {
            // Push a void value on to signify that we're value-less
            asm_push_u64(stream, 0);
        }
        
        asm_single_op(stream, CIO_NEW_BINDING);
    }
    
    return VISIT_OK;
}

CI_VISITOR(AST_OPERATOR, ASTOperator) {
    
    if (phase == VISIT_PRE) {
        assert(AST_BASE(node)->location.ctx);
        uint64_t start = AST_BASE(node)->location.range_start - AST_BASE(node)->location.ctx->buf;
        uint64_t end = AST_BASE(node)->location.range_end - AST_BASE(node)->location.ctx->buf;
        assert (start <= end);
        
        asm_push_u64(stream, AST_BASE(node)->kind);
        asm_push_u64(stream, 0); /* source data index */
        asm_push_u64(stream, start);
        asm_push_u64(stream, end);
        asm_single_op(stream, CIO_NEW_AST_NODE);
    }
    
    return VISIT_OK;
}

CI_VISITOR(AST_EXPR_NUMBER, ASTExprNumber) {
    
    if (phase == VISIT_PRE) {
        asm_push_u64(stream, node->number);
        asm_single_op(stream, CIO_NEW_INTEGER_LITERAL);
    }
    
    return VISIT_OK;
}

CI_VISITOR(AST_EXPR_IDENT, ASTExprIdent) {
    if (phase == VISIT_POST) {
        //uint64_t start = node->base.location.range_start - node->base.location.ctx->buf;
        //uint64_t end = node->base.location.range_end - node->base.location.ctx->buf;
        //asm_new_source_location(stream, start, end);
        // TODO(bloggins): This should probably be CIO_IDENTIFIER_LITERAL!
        asm_single_op(stream, CIO_NEW_IDENTIFIER);
    }
    
    return VISIT_OK;
}

CI_VISITOR(AST_EXPR_BINARY, ASTExprBinary) {
    assert(node->op->op.kind == TOK_PLUS && "only addition supported right now");
    
    if (phase == VISIT_POST) {
        asm_single_op(stream, CIO_BINOP);
    }
    
    return VISIT_OK;
}

CI_VISITOR(AST_EXPR_CALL, ASTExprCall) {
    if (phase == VISIT_POST) {
        size_t arg_count = list_count(node->args);
        asm_push_u64(stream, arg_count);
        asm_single_op(stream, CIO_CALL);
    }
    
    return VISIT_OK;
}


CI_VISITOR(AST_IDENT, ASTIdent) {
    
    return VISIT_OK;
}

CI_VISITOR(AST_TYPE_CONSTANT, ASTTypeConstant) {
    return VISIT_OK;
}

// *** END OVERRIDES SECTION ***

#pragma mark Visitor Routine

VisitFn visitors[] = {
#   define AST(kind, name, type) (VisitFn)ci_visit_AST_##kind,
#   include "ast.def.h"
};
#undef AST


int ci_visit(ASTBase *node, VisitPhase phase, ByteStream* stream) {
    
    if (phase == VISIT_PRE) {
        assert(node->location.ctx);
        uint64_t start = node->location.range_start - node->location.ctx->buf;
        uint64_t end = node->location.range_end - node->location.ctx->buf;
        assert (start <= end);
        
        asm_push_u64(stream, node->kind);
        asm_push_u64(stream, 0); /* source data index */
        asm_push_u64(stream, start);
        asm_push_u64(stream, end);
        asm_single_op(stream, CIO_NEW_AST_NODE);
        
        asm_push_node(stream, node->kind);
    }
    
    int res = visitors[node->kind](node, phase, stream);
    
    if (res == VISIT_OK && phase == VISIT_POST) {
        asm_single_op(stream, CIO_POP_NODE);
    }
    
    return res;
}

#pragma mark Public API

bool interp_interpret(ASTBase *node, ASTBase **result) {
    
    ByteStream *stream = calloc(1, sizeof(ByteStream));
    
    asm_push_u64(stream, 1);
    ASM_OP(CIO_DECLARE_FR_VERSION);
    
    ast_visit(node, (VisitFn)ci_visit, stream);
    ast_visit_data_clean(node);
    
    asm_single_op(stream, CIO_HALT);
    
#   if 0
    fprintf(stderr, "OPCODES: [");
    for (size_t idx = 0; idx < stream->current_offset; idx++) {
        fprintf(stderr, "0x%X, ", stream->data[idx]);
    }
    fprintf(stderr, "]\n");
#   endif

    // "VM"
    uint64_t stack[256] = {};
    uint16_t sp = 1; // 0th is a stack underflow
#   define PUSH(v) stack[sp++] = (v)
#   define POP()   stack[--sp]
    
    uint8_t *ip = stream->data;

    CIValueTable value_table = {};
    CIValue v;
    v.kind = CIV_VOID;
    ValueTableIndex void_idx = values_new(&value_table, &v);
    assert(void_idx == 0);
    
#   define LOOKUP_VALUE(idx) (&value_table.values[(idx)])
    
    // Interpreter
    // TODO(bloggins): Direct thread this with computed gotos
    while (*ip != CIO_HALT) {
        switch (*ip) {
            case CIO_HALT: assert(false && "how did we get here?");
            case CIO_DECLARE_FR_VERSION: {
                ++ip;
                
                uint64_t version = POP();
                assert((version == 1) && "unsupported FR bytecode version");
            } break;
            case CIO_PUSH_U64: {
                ++ip;
                
                uint64_t value = decode_u64(&ip);
                PUSH(value);
            } break;
            case CIO_PUSH_NODE: {
                /* ASTKind kind = * */ ++ip;
                POP();  // Node index from push node
                
                ++ip;
            } break;
            case CIO_POP_NODE: {
                ++ip;
            } break;
            case CIO_BINOP: {
                ValueTableIndex v2_idx = POP();
                ValueTableIndex op_idx = POP();
                ValueTableIndex v1_idx = POP();
                
                assert(v1_idx < value_table.count);
                assert(op_idx < value_table.count);
                assert(v2_idx < value_table.count);
                
                CIValue *v1 = LOOKUP_VALUE(v1_idx);
                CIValue *op = LOOKUP_VALUE(op_idx);
                CIValue *v2 = LOOKUP_VALUE(v2_idx);
                assert(v1);
                assert(op);
                assert(v2);
                
                if (v1->kind == CIV_POD_INTEGER && v2->kind == CIV_POD_INTEGER) {
                    // TODO(bloggins): we can't keep making new values like this!
                    // For most literal values we can probably get away with tagging
                    // the high bits and using the literal bits.
                    //
                    // Or... tie values to scope so we can throw them away when
                    // we no longer need them. That's probably the better answer.
                    CIValue v;
                    v.kind = CIV_POD_INTEGER;
                    v.simple_value.int_value = v1->simple_value.int_value + v2->simple_value.int_value;
                    ValueTableIndex idx = values_new(&value_table, &v);
                    PUSH(idx);
                } else {
                    // We don't really know how to add this type so we'll emit an
                    // unresolved value
                    CIValue v;
                    v.kind = CIV_BINOP;
                    v.binop_value.lhs_index = v1_idx;
                    v.binop_value.op_index = op_idx;
                    v.binop_value.rhs_index = v2_idx;
                    
                    ValueTableIndex idx = values_new(&value_table, &v);
                    PUSH(idx);
                }
                
                ++ip;
            } break;
            case CIO_PUSH_IP: {
                ++ip;
                PUSH(ip - stream->data);
            } break;
            case CIO_CALL: {
                ++ip;
                
                // TODO(bloggins): Push these into a Scope for the call
                uint64_t argcount = POP();
                while (argcount--) {
                    POP();
                }
                
                PUSH(ip - stream->data);
                
                // Jump to function handler
                fprintf(stderr, "Can't call function yet\n");
                exit(ERR_INTERPRET);
               
            } break;
            case CIO_JUMP: {
                uint64_t ip_offset = POP();
                uint8_t *ptr = stream->data + ip_offset;
                assert((ptr < stream->data + stream->current_offset) && "illegal jump address");
            } break;
            case CIO_NEW_INTEGER_LITERAL: {
                CIValue v;
                v.kind = CIV_INTEGER_LITERAL;
                v.simple_value.is_pod = false;
                v.simple_value.int_value = POP();
                ValueTableIndex idx = values_new(&value_table, &v);
                
                PUSH(idx);
                ++ip;
            } break;
            case CIO_NEW_IDENTIFIER: {
                CIValue v;
                v.kind = CIV_IDENTIFIER;
                ValueTableIndex idx = values_new(&value_table, &v);
                value_table.values[idx].identifier_id = idx;
                
                PUSH(idx);
                ++ip;
            } break;
            case CIO_NEW_BINDING: {
                ++ip;
                
                uint64_t value_id = POP();
                uint64_t name_id = POP();
                uint64_t constraint_id = POP();
                
                CIValue v;
                v.kind = CIV_BINDING;
                v.binding_value.constraint_index = constraint_id;
                v.binding_value.name_index = name_id;
                v.binding_value.value_index = value_id;
                
                PUSH(values_new(&value_table, &v));
                
            } break;
            case CIO_NEW_AST_NODE: {
                ++ip;
                
                uint64_t end = POP();
                uint64_t start = POP();
                uint64_t source = POP();
                uint64_t kind = POP();
                
                assert(start <= end);
                assert(kind < AST_LAST);
                
                // TODO(bloggins): yeah... probably want to speed this up
                ValueTableIndex node_idx = 0;
                for (ValueTableIndex idx = 0; idx < value_table.count; idx++) {
                    CIValue *candidate = LOOKUP_VALUE(idx);
                    if (candidate->kind != CIV_AST_NODE) {
                        continue;
                    }
                    
                    if (candidate->ast_node_value.kind == kind &&
                        candidate->ast_node_value.source_index == source &&
                        candidate->ast_node_value.start == start &&
                        candidate->ast_node_value.end == end) {
                        node_idx = idx;
                        break;
                    }
                }
                
                if (node_idx == 0) {
                    CIValue v;
                    v.kind = CIV_AST_NODE;
                    v.ast_node_value.source_index = 0;
                    v.ast_node_value.start = start;
                    v.ast_node_value.end = end;
                    v.ast_node_value.kind = (ASTKind)kind;
                    
                    node_idx = values_new(&value_table, &v);
                }
                PUSH(node_idx);
            } break;
            default: {
                uint8_t op = *ip;
                if (op < CIO_LAST) {
                    fprintf(stderr, "Unrecognized opcode %s\n", g_opcode_names[op]);
                }
                assert(false && "unrecognized opcode");
            }
        }
        
        if (sp < 1) {
            assert("stack underflow");
        } else if (sp > 256) {
            assert("stack overflow");
        }
    }
    
    // DEBUG - Print value table
    /*
    for (size_t i = 0; i < value_table.count; i++) {
        CIValue *v = &value_table.values[i];
        
        const char *type_name = "unknown";
        switch (v->kind) {
            case CIV_VOID: type_name = "void"; break;
            case CIV_CHAR_LITERAL: type_name = "char_literal"; break;
            case CIV_INTEGER_LITERAL: type_name = "integer_literal"; break;
            case CIV_STRING_LITERAL: type_name = "string_literal"; break;
            case CIV_BINDING: type_name = "binding"; break;
            case CIV_BINOP: type_name = "binop"; break;
            case CIV_IDENTIFIER: type_name = "identifier"; break;
            case CIV_AST_NODE: type_name = "ast"; break;
            case CIV_LAST: default: assert(false && "unrecognized value kind");
        }
        
        fprintf(stderr, "%zu: <%s> ", i, type_name);
        
        ci_value_fprint(stderr, node->location.ctx, v);
        fprintf(stderr, "\n");
    }
    */
    
    // Hopefully the last value will be main result of the interpretation
    ci_value_fprint(stderr, node->location.ctx, &value_table.values[value_table.count - 1]);
    fprintf(stderr, "\n");
    
    return true;
}

