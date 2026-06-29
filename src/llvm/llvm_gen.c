#include "aether/llvm.h"
#include "aether/str.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* ──────────────────────────────────────────────
 * Main codegen entry point
 *
 * Walks the AST program and generates LLVM IR
 * for all top-level declarations.
 * ────────────────────────────────────────────── */
bool llvm_generate(LlvmCodegen *lc, AstNode *program) {
    if (!program || program->type != NODE_PROGRAM) {
        fprintf(stderr, "LLVM: expected NODE_PROGRAM\n");
        return false;
    }

    /* Declare runtime functions */
    llvm_declare_runtime(lc);

    /* Single pass: generate function bodies.
     * Forward references are handled by LLVM's module-level symbol table. */
    for (int i = 0; i < program->data.list.count; i++) {
        AstNode *decl = program->data.list.items[i];
        if (decl->type == NODE_FUNC_DECL) {
            llvm_cg_func_decl(lc, decl);
        }
    }

    return true;
}
