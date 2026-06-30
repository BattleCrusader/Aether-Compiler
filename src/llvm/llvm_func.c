#include "aether/llvm.h"
#include "aether/str.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ──────────────────────────────────────────────
 * Function codegen
 * ────────────────────────────────────────────── */

/* ──────────────────────────────────────────────
 * Generate code for a function declaration
 * ────────────────────────────────────────────── */
void llvm_cg_func_decl(LlvmCodegen *lc, AstNode *node) {
    if (node->type != NODE_FUNC_DECL) return;

    StringView fname = node->data.func.name->data.ident.name;
    int param_count = node->data.func.params.count;
    AstNode *return_type_node = node->data.func.return_type;
    AstNode *body = node->data.func.body;

    /* Build function name */
    char name[256];
    int nlen = (int)fname.len;
    if (nlen > 255) nlen = 255;
    memcpy(name, fname.data, nlen);
    name[nlen] = '\0';

    /* Build parameter types */
    LLVMTypeRef *param_types = NULL;
    if (param_count > 0) {
        param_types = (LLVMTypeRef *)malloc(param_count * sizeof(LLVMTypeRef));
        for (int i = 0; i < param_count; i++) {
            AstNode *param_type_node = node->data.func.params.items[i]->data.param.type;
            if (param_type_node) {
                param_types[i] = llvm_type_from_ast(lc, param_type_node);
            } else {
                param_types[i] = LLVMInt64TypeInContext(lc->context);
            }
        }
    }

    /* Return type */
    LLVMTypeRef ret_type = return_type_node
        ? llvm_type_from_ast(lc, return_type_node)
        : LLVMVoidTypeInContext(lc->context);

    /* Create function type */
    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, param_count, false);

    /* Create the function (or get existing from first pass) */
    LLVMValueRef func = LLVMGetNamedFunction(lc->module, name);
    if (!func) {
        func = LLVMAddFunction(lc->module, name, func_type);
    }
    lc->current_func = func;

    /* Create entry block */
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(lc->builder, entry);
    lc->current_block = entry;

    /* Store parameters in alloca'd slots */
    for (int i = 0; i < param_count; i++) {
        AstNode *param = node->data.func.params.items[i];
        StringView pname = param->data.param.name->data.ident.name;

        LLVMValueRef alloca = LLVMBuildAlloca(lc->builder, param_types[i], "");
        LLVMBuildStore(lc->builder, LLVMGetParam(func, i), alloca);

        llvm_declare_local(lc, pname, alloca, param_types[i], param->data.param.is_mut);
    }

    /* Generate body */
    if (body) {
        llvm_cg_block(lc, body);
    }

    /* If the function doesn't end with a return, add one */
    LLVMBasicBlockRef current = LLVMGetInsertBlock(lc->builder);
    if (current && LLVMGetBasicBlockTerminator(current) == NULL) {
        if (ret_type == LLVMVoidTypeInContext(lc->context)) {
            LLVMBuildRetVoid(lc->builder);
        } else {
            LLVMBuildRet(lc->builder,
                LLVMConstInt(ret_type, 0, false));
        }
    }

    /* Verify the function */
    char *error = NULL;
    LLVMVerifyFunction(func, LLVMPrintMessageAction);
    if (error) {
        fprintf(stderr, "LLVM function verification failed: %s\n", error);
        LLVMDisposeMessage(error);
    }

    free(param_types);
}
