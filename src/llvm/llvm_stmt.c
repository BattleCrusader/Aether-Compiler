#include "aether/llvm.h"
#include "aether/str.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ──────────────────────────────────────────────
 * Statement codegen
 * ────────────────────────────────────────────── */

/* Forward declarations */
static void cg_let(LlvmCodegen *lc, AstNode *node);
static void cg_return(LlvmCodegen *lc, AstNode *node);
static void cg_expr_stmt(LlvmCodegen *lc, AstNode *node);

/* ──────────────────────────────────────────────
 * Main statement dispatcher
 * ────────────────────────────────────────────── */
void llvm_cg_stmt(LlvmCodegen *lc, AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_LET:       cg_let(lc, node); break;
        case NODE_RETURN:    cg_return(lc, node); break;
        case NODE_EXPR_STMT: cg_expr_stmt(lc, node); break;
        case NODE_BLOCK:     llvm_cg_block(lc, node); break;
        default:
            fprintf(stderr, "LLVM: unhandled statement node type %d\n", node->type);
            break;
    }
}

/* ──────────────────────────────────────────────
 * Block — iterate statements
 * ────────────────────────────────────────────── */
void llvm_cg_block(LlvmCodegen *lc, AstNode *node) {
    for (int i = 0; i < node->data.list.count; i++) {
        llvm_cg_stmt(lc, node->data.list.items[i]);
    }
}

/* ──────────────────────────────────────────────
 * Let declaration — alloca + store
 * ────────────────────────────────────────────── */
static void cg_let(LlvmCodegen *lc, AstNode *node) {
    StringView name = node->data.let_decl.name->data.ident.name;
    AstNode *type_node = node->data.let_decl.type;
    AstNode *value_node = node->data.let_decl.value;

    /* Determine LLVM type */
    LLVMTypeRef llvm_type;
    if (type_node) {
        llvm_type = llvm_type_from_ast(lc, type_node);
    } else {
        /* Inferred type — default to i64 for now */
        llvm_type = LLVMInt64TypeInContext(lc->context);
    }

    /* Allocate stack slot */
    LLVMValueRef alloca = LLVMBuildAlloca(lc->builder, llvm_type, "");
    char name_buf[256];
    int nlen = (int)name.len;
    if (nlen > 255) nlen = 255;
    memcpy(name_buf, name.data, nlen);
    name_buf[nlen] = '\0';
    LLVMSetValueName2(alloca, name_buf, nlen);

    /* Store initial value if provided */
    if (value_node) {
        LLVMValueRef val = llvm_cg_expr(lc, value_node);
        LLVMBuildStore(lc->builder, val, alloca);
    }

    /* Register in symbol table */
    llvm_declare_local(lc, name, alloca, llvm_type, node->data.let_decl.is_mut);
}

/* ──────────────────────────────────────────────
 * Return statement
 * ────────────────────────────────────────────── */
static void cg_return(LlvmCodegen *lc, AstNode *node) {
    if (node->data.return_node.value) {
        LLVMValueRef val = llvm_cg_expr(lc, node->data.return_node.value);
        LLVMBuildRet(lc->builder, val);
    } else {
        LLVMBuildRetVoid(lc->builder);
    }
}

/* ──────────────────────────────────────────────
 * Expression statement — evaluate and discard
 * ────────────────────────────────────────────── */
static void cg_expr_stmt(LlvmCodegen *lc, AstNode *node) {
    /* NODE_EXPR_STMT stores the expression in data.call.callee */
    if (node->data.call.callee) {
        llvm_cg_expr(lc, node->data.call.callee);
    }
}
