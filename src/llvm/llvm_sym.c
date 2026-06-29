#include "aether/llvm.h"
#include <string.h>
#include <stdio.h>

/* ──────────────────────────────────────────────
 * Symbol table helpers
 *
 * Simple dynamic arrays for local and global
 * symbol lookups. Linear search — fine for the
 * typical function size (< 100 locals).
 * ────────────────────────────────────────────── */

/* ──────────────────────────────────────────────
 * Declare a local variable
 * ────────────────────────────────────────────── */
void llvm_declare_local(LlvmCodegen *lc, StringView name,
    LLVMValueRef value, LLVMTypeRef type, bool is_mut)
{
    if (lc->local_count >= lc->local_cap) {
        int new_cap = lc->local_cap ? lc->local_cap * 2 : 16;
        LlvmSymbol *new_syms = (LlvmSymbol *)arena_alloc(lc->arena,
            new_cap * sizeof(LlvmSymbol));
        if (lc->locals) {
            memcpy(new_syms, lc->locals, lc->local_count * sizeof(LlvmSymbol));
        }
        lc->locals = new_syms;
        lc->local_cap = new_cap;
    }

    lc->locals[lc->local_count].name = name;
    lc->locals[lc->local_count].value = value;
    lc->locals[lc->local_count].type = type;
    lc->locals[lc->local_count].is_mutable = is_mut;
    lc->local_count++;
}

/* ──────────────────────────────────────────────
 * Look up a local variable by name
 * Returns NULL if not found.
 * ────────────────────────────────────────────── */
LLVMValueRef llvm_lookup_local(LlvmCodegen *lc, StringView name) {
    for (int i = lc->local_count - 1; i >= 0; i--) {
        if (sv_eq(lc->locals[i].name, name)) {
            return lc->locals[i].value;
        }
    }
    return NULL;
}

/* ──────────────────────────────────────────────
 * Declare a global symbol
 * ────────────────────────────────────────────── */
void llvm_declare_global(LlvmCodegen *lc, StringView name,
    LLVMValueRef value, LLVMTypeRef type)
{
    if (lc->global_count >= lc->global_cap) {
        int new_cap = lc->global_cap ? lc->global_cap * 2 : 16;
        LlvmSymbol *new_syms = (LlvmSymbol *)arena_alloc(lc->arena,
            new_cap * sizeof(LlvmSymbol));
        if (lc->globals) {
            memcpy(new_syms, lc->globals, lc->global_count * sizeof(LlvmSymbol));
        }
        lc->globals = new_syms;
        lc->global_cap = new_cap;
    }

    lc->globals[lc->global_count].name = name;
    lc->globals[lc->global_count].value = value;
    lc->globals[lc->global_count].type = type;
    lc->globals[lc->global_count].is_mutable = false;
    lc->global_count++;
}

/* ──────────────────────────────────────────────
 * Look up a global symbol by name
 * Returns NULL if not found.
 * ────────────────────────────────────────────── */
LLVMValueRef llvm_lookup_global(LlvmCodegen *lc, StringView name) {
    for (int i = 0; i < lc->global_count; i++) {
        if (sv_eq(lc->globals[i].name, name)) {
            return lc->globals[i].value;
        }
    }
    return NULL;
}
