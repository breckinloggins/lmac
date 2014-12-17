//
//  ci_opcodes.def.h
//  lmac
//
//  Created by Breckin Loggins on 12/16/14.
//  Copyright (c) 2014 Breckin Loggins. All rights reserved.
//

// Opcodes for the Front-end Representation (FR) virtual machine

#ifndef CI_OP
#define CI_OP(kind)
#endif

/* Opcode Description Format:
 * n:t desc (extended desc)
 *  n = argument number from 1 (0th argument is considered the opcode itself)
 *  t = encoding type (like u64 or i32)
 *  desc = short arg name
 *  extended desc = long arg description
 *
 * ->t name (extended desc)
 *  push an argument of type t on the stack. Push in top-down order.
 *
 * <-t name (extended desc)
 *  op will push an argument of type t on the stack. Will push in top-down order
 */


/* must be the first to ensure improper NULLs halt */
CI_OP(CIO_HALT)

/* ->u64 version (FR bytecode instruction format version) */
CI_OP(CIO_DECLARE_FR_VERSION)

/* 1:u64 value
 *
 * <-u64 value
 */
CI_OP(CIO_PUSH_U64)

CI_OP(CIO_PUSH_NODE)  /* kind, sl_start */
CI_OP(CIO_POP_NODE)   /* sl_end */

/* ->u64 integer (the integer value as determined by the lexer or parser)
 *
 * <-u64 value_id (index into the value table)
 */
CI_OP(CIO_NEW_INTEGER_LITERAL)

/* <-u64 identifier_id (index into the value table) */
CI_OP(CIO_NEW_IDENTIFIER)

/* ->u64 constraint_id (index into the value table)
 * ->u64 name_id (index into the value table)
 * ->u64 value_id (index in the value table)
 *
 * <-u64 binding_id (index in the value table)
 */
CI_OP(CIO_NEW_BINDING)

/* ->u64 kind (ASTKind value)
 * ->u64 source_id (index into the value table)
 * ->u64 start (offset into the source data)
 * ->u64 end (offset into the source data)
 *
 * <-u64 node_id (index in the value table)
 *
 * note: if a node already exists with the given parameters, 
 *       the existing ID will be returned
 */
CI_OP(CIO_NEW_AST_NODE)

/* Control flow */
CI_OP(CIO_PUSH_IP)    /* no-args, pushes instruction pointer on the stack */
CI_OP(CIO_JUMP)       /* no-args, jumps to the instruction at the address on the top of the stack */

/*
 * ->u64 lhs_value_id (index into the value table)
 * ->u64 op_id (index into the value table)
 * ->u64 rhs_value_id (index into the value table)
 *
 * <-u64 result_id (index into the value table)
 *
 * TODO(bloggins): Should we replace this with stack op primitives and CIO_CALL?
 */
CI_OP(CIO_BINOP)

/* ->u64 callable_id (index into the value table)
 * ->u64 arg_id_first (index into the value table)
 *     ...
 * ->u64 arg_id_last
 * ->u64 arg_count
 * ->864 callable_id (the same as the first)
 *
 * <-u64 result_id (index into the value table)
 */
CI_OP(CIO_CALL)

/* must be last */
CI_OP(CIO_LAST)



#undef CI_OP