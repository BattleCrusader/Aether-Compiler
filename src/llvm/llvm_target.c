#include "aether/llvm.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ──────────────────────────────────────────────
 * Target setup and object/assembly/binary emission
 * ────────────────────────────────────────────── */

/* ──────────────────────────────────────────────
 * Create a target machine for the given target
 * ────────────────────────────────────────────── */
static LLVMTargetMachineRef create_target_machine(LlvmCodegen *lc) {
    const char *triple = llvm_target_triple(lc->target);
    const char *cpu = "generic";
    const char *features = "";

    LLVMTargetRef target = NULL;
    char *error = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "LLVM: failed to get target for triple '%s': %s\n", triple, error ? error : "unknown");
        if (error) LLVMDisposeMessage(error);
        return NULL;
    }

    LLVMCodeGenOptLevel opt_level;
    switch (lc->opt_level) {
        case 0: opt_level = LLVMCodeGenLevelNone; break;
        case 1: opt_level = LLVMCodeGenLevelLess; break;
        case 2: opt_level = LLVMCodeGenLevelDefault; break;
        case 3: opt_level = LLVMCodeGenLevelAggressive; break;
        default: opt_level = LLVMCodeGenLevelNone; break;
    }

    LLVMRelocMode reloc = LLVMRelocDefault;
    LLVMCodeModel code_model = LLVMCodeModelDefault;

    LLVMTargetMachineRef tm = LLVMCreateTargetMachine(target, triple, cpu, features,
        opt_level, reloc, code_model);
    return tm;
}

/* ──────────────────────────────────────────────
 * Emit object file (.o)
 * Returns 0 on success, nonzero on error.
 * ────────────────────────────────────────────── */
int llvm_emit_object(LlvmCodegen *lc, const char *path) {
    LLVMTargetMachineRef tm = create_target_machine(lc);
    if (!tm) return 1;

    /* Emit object file directly without pass manager */
    char *error = NULL;
    int result = LLVMTargetMachineEmitToFile(tm, lc->module, (char *)path,
        LLVMObjectFile, &error);
    if (result != 0) {
        fprintf(stderr, "LLVM: failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
    }

    LLVMDisposeTargetMachine(tm);
    return result;
}

/* ──────────────────────────────────────────────
 * Emit assembly listing (.asm or .ll)
 * Returns 0 on success, nonzero on error.
 * ────────────────────────────────────────────── */
int llvm_emit_assembly(LlvmCodegen *lc, const char *path) {
    LLVMTargetMachineRef tm = create_target_machine(lc);
    if (!tm) return 1;

    char *error = NULL;
    int result = LLVMTargetMachineEmitToFile(tm, lc->module, (char *)path,
        LLVMAssemblyFile, &error);
    if (result != 0) {
        fprintf(stderr, "LLVM: failed to emit assembly: %s\n", error);
        LLVMDisposeMessage(error);
    }

    LLVMDisposeTargetMachine(tm);
    return result;
}

/* ──────────────────────────────────────────────
 * Emit flat binary (for boot targets)
 * Uses LLVM → ELF → llvm-objcopy pipeline.
 * Returns 0 on success, nonzero on error.
 * ────────────────────────────────────────────── */
int llvm_emit_binary(LlvmCodegen *lc, const char *path) {
    /* First emit as ELF object */
    char obj_path[1024];
    snprintf(obj_path, sizeof(obj_path), "%s.o", path);

    if (llvm_emit_object(lc, obj_path) != 0) {
        return 1;
    }

    /* Use llvm-objcopy to convert to flat binary */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "llvm-objcopy -O binary %s %s", obj_path, path);
    int result = system(cmd);

    /* Clean up the intermediate .o file */
    snprintf(cmd, sizeof(cmd), "rm -f %s", obj_path);
    system(cmd);

    if (result != 0) {
        fprintf(stderr, "LLVM: llvm-objcopy failed (exit code %d)\n", result);
        return 1;
    }

    return 0;
}

/* ──────────────────────────────────────────────
 * Generate linker script for @layout / @kernel_layout
 * Returns heap-allocated string, caller must free.
 * ────────────────────────────────────────────── */
char *llvm_generate_linker_script(LlvmCodegen *lc) {
    /* For now, return a basic linker script */
    const char *script =
        "SECTIONS\n"
        "{\n"
        "    . = 0x7C00;\n"
        "    .text : { *(.text) }\n"
        "    .rodata : { *(.rodata) }\n"
        "    .data : { *(.data) }\n"
        "    .bss : { *(.bss) }\n"
        "}\n";

    return strdup(script);
}
