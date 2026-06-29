#include "aether/llvm.h"
#include "aether/str.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ──────────────────────────────────────────────
 * Forward declarations for expression codegen
 * Each expression type has its own function.
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_literal_int(LlvmCodegen *lc, AstNode *node);
static LLVMValueRef cg_literal_float(LlvmCodegen *lc, AstNode *node);
static LLVMValueRef cg_literal_string(LlvmCodegen *lc, AstNode *node);
static LLVMValueRef cg_literal_bool(LlvmCodegen *lc, AstNode *node);
static LLVMValueRef cg_ident(LlvmCodegen *lc, AstNode *node);
static LLVMValueRef cg_binary_op(LlvmCodegen *lc, AstNode *node);
static LLVMValueRef cg_call(LlvmCodegen *lc, AstNode *node);

/* ──────────────────────────────────────────────
 * Main expression dispatcher
 * ────────────────────────────────────────────── */
LLVMValueRef llvm_cg_expr(LlvmCodegen *lc, AstNode *node) {
    if (!node) return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);

    switch (node->type) {
        case NODE_LITERAL_INT:    return cg_literal_int(lc, node);
        case NODE_LITERAL_FLOAT:  return cg_literal_float(lc, node);
        case NODE_LITERAL_STRING: return cg_literal_string(lc, node);
        case NODE_LITERAL_BOOL:   return cg_literal_bool(lc, node);
        case NODE_LITERAL_CHAR:   return LLVMConstInt(LLVMInt8TypeInContext(lc->context),
                                        node->data.literal.char_val, false);
        case NODE_LITERAL_NONE:   return LLVMConstNull(LLVMPointerType(
                                        LLVMInt8TypeInContext(lc->context), 0));
        case NODE_IDENT:          return cg_ident(lc, node);
        case NODE_BINARY_OP:      return cg_binary_op(lc, node);
        case NODE_CALL:           return cg_call(lc, node);
        default:
            fprintf(stderr, "LLVM: unhandled expression node type %d\n", node->type);
            return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);
    }
}

/* ──────────────────────────────────────────────
 * Integer literal
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_literal_int(LlvmCodegen *lc, AstNode *node) {
    uint64_t val = node->data.literal.int_val;
    if (node->data.literal.is_negative) {
        /* Negate manually to avoid compiler warnings about negating min value */
        val = ~val + 1;
    }
    return LLVMConstInt(LLVMInt64TypeInContext(lc->context), val, false);
}

/* ──────────────────────────────────────────────
 * Float literal
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_literal_float(LlvmCodegen *lc, AstNode *node) {
    return LLVMConstReal(LLVMDoubleTypeInContext(lc->context),
        node->data.literal.float_val);
}

/* ──────────────────────────────────────────────
 * String literal — create a global string constant
 * and return a pointer to it.
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_literal_string(LlvmCodegen *lc, AstNode *node) {
    StringView sv = node->data.literal.string_val;
    int len = (int)sv.len;

    /* Create a global constant with a unique name */
    static int str_counter = 0;
    char name[64];
    snprintf(name, sizeof(name), ".str.%d", str_counter++);

    LLVMTypeRef arr_type = LLVMArrayType(LLVMInt8TypeInContext(lc->context), len + 1);
    LLVMValueRef global = LLVMAddGlobal(lc->module, arr_type, name);
    LLVMSetInitializer(global, LLVMConstString(sv.data, len, true));
    LLVMSetGlobalConstant(global, true);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetUnnamedAddr(global, true);

    /* GEP to get a pointer to the first element */
    LLVMValueRef indices[2] = {
        LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false),
        LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false)
    };
    return LLVMConstGEP2(arr_type, global, indices, 2);
}

/* ──────────────────────────────────────────────
 * Boolean literal
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_literal_bool(LlvmCodegen *lc, AstNode *node) {
    return LLVMConstInt(LLVMInt1TypeInContext(lc->context),
        node->data.literal.bool_val ? 1 : 0, false);
}

/* ──────────────────────────────────────────────
 * Identifier — look up in locals, then globals
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_ident(LlvmCodegen *lc, AstNode *node) {
    StringView name = node->data.ident.name;

    /* Try locals first */
    LLVMValueRef local = llvm_lookup_local(lc, name);
    if (local) {
        return LLVMBuildLoad2(lc->builder, LLVMTypeOf(local), local, "");
    }

    /* Try globals */
    LLVMValueRef global = llvm_lookup_global(lc, name);
    if (global) {
        return LLVMBuildLoad2(lc->builder, LLVMTypeOf(global), global, "");
    }

    fprintf(stderr, "LLVM: undefined identifier '%.*s'\n", (int)name.len, name.data);
    return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);
}

/* ──────────────────────────────────────────────
 * Binary operation
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_binary_op(LlvmCodegen *lc, AstNode *node) {
    LLVMValueRef L = llvm_cg_expr(lc, node->data.binary.left);
    LLVMValueRef R = llvm_cg_expr(lc, node->data.binary.right);
    LLVMBuilderRef B = lc->builder;

    switch (node->data.binary.op) {
        case BIN_ADD: return LLVMBuildAdd(B, L, R, "addtmp");
        case BIN_SUB: return LLVMBuildSub(B, L, R, "subtmp");
        case BIN_MUL: return LLVMBuildMul(B, L, R, "multmp");
        case BIN_DIV: return LLVMBuildSDiv(B, L, R, "divtmp");
        case BIN_MOD: return LLVMBuildSRem(B, L, R, "modtmp");
        case BIN_EQ:  return LLVMBuildICmp(B, LLVMIntEQ, L, R, "eqtmp");
        case BIN_NEQ: return LLVMBuildICmp(B, LLVMIntNE, L, R, "neqtmp");
        case BIN_LT:  return LLVMBuildICmp(B, LLVMIntSLT, L, R, "lttmp");
        case BIN_GT:  return LLVMBuildICmp(B, LLVMIntSGT, L, R, "gttmp");
        case BIN_LE:  return LLVMBuildICmp(B, LLVMIntSLE, L, R, "letmp");
        case BIN_GE:  return LLVMBuildICmp(B, LLVMIntSGE, L, R, "getmp");
        case BIN_BIT_AND: return LLVMBuildAnd(B, L, R, "andtmp");
        case BIN_BIT_OR:  return LLVMBuildOr(B, L, R, "ortmp");
        case BIN_BIT_XOR: return LLVMBuildXor(B, L, R, "xortmp");
        case BIN_SHL: return LLVMBuildShl(B, L, R, "shltmp");
        case BIN_SHR: return LLVMBuildAShr(B, L, R, "shrtmp");
        case BIN_AND: return LLVMBuildAnd(B, L, R, "andtmp");
        case BIN_OR:  return LLVMBuildOr(B, L, R, "ortmp");
        default:
            fprintf(stderr, "LLVM: unhandled binary op %d\n", node->data.binary.op);
            return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);
    }
}

/* ──────────────────────────────────────────────
 * Function call — handles print() built-in and
 * regular function calls.
 * ────────────────────────────────────────────── */
static LLVMValueRef cg_call(LlvmCodegen *lc, AstNode *node) {
    if (!node->data.call.callee || node->data.call.callee->type != NODE_IDENT) {
        fprintf(stderr, "LLVM: call with non-ident callee not yet supported\n");
        return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);
    }

    StringView fname = node->data.call.callee->data.ident.name;
    int argc = node->data.call.args.count;

    /* Built-in: print(string) */
    if (sv_eq_cstr(fname, "print") && argc >= 1) {
        /* For now, print is a no-op in LLVM mode.
         * We'll implement it properly when we have the runtime. */
        return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);
    }

    /* Regular function call — look up the function in the module */
    char name[256];
    int nlen = (int)fname.len;
    if (nlen > 255) nlen = 255;
    memcpy(name, fname.data, nlen);
    name[nlen] = '\0';

    LLVMValueRef func = LLVMGetNamedFunction(lc->module, name);
    if (!func) {
        fprintf(stderr, "LLVM: undefined function '%s'\n", name);
        return LLVMConstInt(LLVMInt64TypeInContext(lc->context), 0, false);
    }

    /* Evaluate arguments */
    LLVMValueRef *args = (LLVMValueRef *)malloc(argc * sizeof(LLVMValueRef));
    for (int i = 0; i < argc; i++) {
        args[i] = llvm_cg_expr(lc, node->data.call.args.items[i]);
    }

    LLVMValueRef result = LLVMBuildCall2(lc->builder,
        LLVMGetElementType(LLVMTypeOf(func)), func, args, argc, "calltmp");
    free(args);
    return result;
}
