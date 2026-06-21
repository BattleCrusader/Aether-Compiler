/* ================================================================
 * universal.c — Universal Binary Builder Implementation
 *
 * Generates CPU detection trampolines, assembles per-architecture
 * code, and merges into a single multi-arch ELF binary.
 * ================================================================ */

#include "aether/universal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --- Initialize config with defaults --- */
void universal_config_init(UniversalConfig *config) {
    memset(config, 0, sizeof(UniversalConfig));
    config->arch_flags = (1 << ARCH_X86_64) | (1 << ARCH_ARM64);
    config->x86_64_assembler = "nasm";
    config->arm64_assembler = "aarch64-linux-gnu-as";
    config->riscv64_assembler = "riscv64-linux-gnu-as";
    config->linker = "x86_64-elf-ld";
    config->output_file = "a.out";
}

void universal_config_set_archs(UniversalConfig *config, int arch_mask) {
    config->arch_flags = arch_mask;
}

/* --- Create/destroy builder --- */
UniversalBuilder *universal_builder_create(const UniversalConfig *config) {
    UniversalBuilder *b = (UniversalBuilder *)calloc(1, sizeof(UniversalBuilder));
    if (!b) return NULL;
    if (config) b->config = *config;
    else universal_config_init(&b->config);
    return b;
}

void universal_builder_destroy(UniversalBuilder *builder) {
    if (!builder) return;
    free(builder->asm_x86_64);
    free(builder->asm_arm64);
    free(builder->asm_riscv64);
    free(builder);
}

/* --- Add per-architecture assembly --- */
int universal_builder_add_asm(UniversalBuilder *builder, ArchId arch,
                               const char *asm_text, size_t asm_len) {
    char **dest = NULL;
    size_t *dest_len = NULL;
    switch (arch) {
        case ARCH_X86_64:  dest = &builder->asm_x86_64; dest_len = &builder->asm_x86_64_len; break;
        case ARCH_ARM64:   dest = &builder->asm_arm64;  dest_len = &builder->asm_arm64_len; break;
        case ARCH_RISCV64: dest = &builder->asm_riscv64; dest_len = &builder->asm_riscv64_len; break;
        default: return -1;
    }
    free(*dest);
    *dest = (char *)malloc(asm_len + 1);
    if (!*dest) return -1;
    memcpy(*dest, asm_text, asm_len);
    (*dest)[asm_len] = '\0';
    *dest_len = asm_len;
    return 0;
}

/* --- Architecture names --- */
const char *arch_name(ArchId arch) {
    switch (arch) {
        case ARCH_X86_64:  return "x86_64";
        case ARCH_ARM64:   return "arm64";
        case ARCH_RISCV64: return "riscv64";
        default: return "unknown";
    }
}

const char *arch_asm_section_name(ArchId arch) {
    switch (arch) {
        case ARCH_X86_64:  return ".text.x86_64";
        case ARCH_ARM64:   return ".text.arm64";
        case ARCH_RISCV64: return ".text.riscv64";
        default: return ".text.unknown";
    }
}

int arch_asm_flags(ArchId arch) {
    return (1 << arch);
}

/* --- Generate x86_64 CPUID trampoline --- */
size_t universal_trampoline_x86_64(char *buf, size_t buf_size,
                                    const char *arm64_entry) {
    int n = snprintf(buf, buf_size,
        "; CPU detection trampoline\n"
        "section .text.trampoline\n"
        "extern _aether_entry\n"
        "extern _start_arm64\n"
        "global _start\n"
        "_start:\n"
        "    ; Try to toggle EFLAGS.ID bit (bit 21)\n"
        "    pushfq\n"
        "    pushfq\n"
        "    xor qword [rsp], 0x00200000\n"
        "    popfq\n"
        "    pushfq\n"
        "    pop rax\n"
        "    xor rax, [rsp]\n"
        "    and rax, 0x00200000\n"
        "    popfq\n"
        "    test rax, rax\n"
        "    jnz _start_x86\n"
        "    ; Not x86_64 — jump to ARM64 entry\n"
        "    jmp _start_arm64\n"
        "_start_x86:\n"
        "    ; x86_64 — jump to aether entry\n"
        "    jmp _aether_entry\n");
    if (n < 0 || (size_t)n >= buf_size) return 0;
    return (size_t)n;
}

/* --- Generate ARM64 entry point stub (GNU as syntax) --- */
size_t universal_trampoline_arm64(char *buf, size_t buf_size,
                                    const char *arm64_entry) {
    int n = snprintf(buf, buf_size,
        ".arch armv8-a\n"
        ".text\n"
        ".globl _start_arm64\n"
        "_start_arm64:\n"
        "    b %s\n",
        arm64_entry ? arm64_entry : "_start_arm64_main");
    if (n < 0 || (size_t)n >= buf_size) return 0;
    return (size_t)n;
}

/* --- Generate RISC-V entry point stub (GNU as syntax) --- */
size_t universal_trampoline_riscv64(char *buf, size_t buf_size,
                                      const char *riscv64_entry) {
    int n = snprintf(buf, buf_size,
        ".text\n"
        ".globl _start_riscv64\n"
        "_start_riscv64:\n"
        "    j %s\n",
        riscv64_entry ? riscv64_entry : "_start_riscv64_main");
    if (n < 0 || (size_t)n >= buf_size) return 0;
    return (size_t)n;
}

/* --- Generate the full trampoline --- */
int universal_generate_trampoline(UniversalBuilder *builder) {
    char buf[4096];
    size_t pos = 0;

    /* Generate the x86_64 CPUID trampoline (this is the ELF entry point) */
    int has_arm64 = (builder->config.arch_flags & (1 << ARCH_ARM64)) != 0;
    int has_riscv64 = (builder->config.arch_flags & (1 << ARCH_RISCV64)) != 0;

    size_t n = snprintf(buf, sizeof(buf),
        "; CPU detection trampoline\n"
        "section .text.trampoline\n"
        "extern _aether_entry\n"
        "%s"
        "%s"
        "global _start\n"
        "_start:\n"
        "    ; Try to toggle EFLAGS.ID bit (bit 21)\n"
        "    pushfq\n"
        "    pushfq\n"
        "    xor qword [rsp], 0x00200000\n"
        "    popfq\n"
        "    pushfq\n"
        "    pop rax\n"
        "    xor rax, [rsp]\n"
        "    and rax, 0x00200000\n"
        "    popfq\n"
        "    test rax, rax\n"
        "    jnz _start_x86\n"
        "    ; Not x86_64 — jump to ARM64 entry\n"
        "    jmp %s\n"
        "_start_x86:\n"
        "    ; x86_64 — jump to aether entry\n"
        "    jmp _aether_entry\n",
        has_arm64 ? "extern _start_arm64\n" : "",
        has_riscv64 ? "extern _start_riscv64\n" : "",
        has_arm64 ? "_start_arm64" : (has_riscv64 ? "_start_riscv64" : "_aether_entry"));
    if (n < 0 || (size_t)n >= sizeof(buf)) return -1;
    pos = (size_t)n;

    /* Store the trampoline */
    memcpy(builder->trampoline_asm, buf, pos);
    builder->trampoline_len = pos;
    return 0;
}

/* --- Write a temp file --- */
static int write_temp_file(const char *path, const char *content, size_t len) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    return (written == len) ? 0 : -1;
}

/* --- Assemble a single architecture --- */
static int assemble_arch(UniversalBuilder *builder, ArchId arch,
                          const char *asm_text, size_t asm_len,
                          const char *obj_path) {
    /* Write assembly to temp file */
    char asm_path[1024];
    snprintf(asm_path, sizeof(asm_path), "/tmp/aether_universal_%s.asm", arch_name(arch));
    if (write_temp_file(asm_path, asm_text, asm_len) != 0) {
        fprintf(stderr, "Error: cannot write %s\n", asm_path);
        return -1;
    }

    /* Choose assembler */
    const char *assembler = NULL;
    const char *format = NULL;
    switch (arch) {
        case ARCH_X86_64:
            assembler = builder->config.x86_64_assembler;
            format = "elf64";
            break;
        case ARCH_ARM64:
            assembler = builder->config.arm64_assembler;
            format = "elf64";
            break;
        case ARCH_RISCV64:
            assembler = builder->config.riscv64_assembler;
            format = "elf64";
            break;
        default:
            return -1;
    }

    /* Build command */
    char cmd[4096];
    if (arch == ARCH_X86_64) {
        /* Use nasm for x86_64 */
        snprintf(cmd, sizeof(cmd), "nasm -f %s -o %s %s", format, obj_path, asm_path);
    } else {
        /* Use GNU assembler for ARM64/RISC-V */
        snprintf(cmd, sizeof(cmd), "%s -o %s %s 2>/dev/null", assembler, obj_path, asm_path);
    }

    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error: %s assembly failed for %s (exit %d)\n",
                assembler, arch_name(arch), ret);
        /* Try fallback: use nasm with -f elf64 for x86_64, then translate */
        if (arch != ARCH_X86_64) {
            fprintf(stderr, "  Tip: install %s or use --target asm-%s to emit assembly only\n",
                    assembler, arch_name(arch));
        }
        return -1;
    }

    remove(asm_path);
    return 0;
}

/* --- Build the universal binary --- */
int universal_build(UniversalBuilder *builder, const char *output_file) {
    /* Step 1: Generate trampoline if not already done */
    if (builder->trampoline_len == 0) {
        if (universal_generate_trampoline(builder) != 0) {
            fprintf(stderr, "Error: failed to generate CPU detection trampoline\n");
            return -1;
        }
    }

    /* Step 2: Assemble each architecture — track which actually succeed */
    char obj_files[3][1024];
    int arch_count = 0;
    ArchId archs[3];
    int actual_flags = 0;

    if (builder->config.arch_flags & (1 << ARCH_X86_64)) {
        snprintf(obj_files[arch_count], sizeof(obj_files[arch_count]),
                 "/tmp/aether_universal_x86_64.o");
        char asm_path[1024];
        snprintf(asm_path, sizeof(asm_path), "/tmp/aether_universal_x86_64.asm");
        FILE *f = fopen(asm_path, "w");
        if (f) { fwrite(builder->asm_x86_64, 1, builder->asm_x86_64_len, f); fclose(f); }
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "nasm -f elf64 -o %s %s", obj_files[arch_count], asm_path);
        int ret = system(cmd);
        if (ret != 0) {
            fprintf(stderr, "Error: x86_64 assembly failed (exit %d)\n", ret);
            return -1;
        }
        remove(asm_path);
        archs[arch_count] = ARCH_X86_64;
        actual_flags |= (1 << ARCH_X86_64);
        arch_count++;
    }

    if (builder->config.arch_flags & (1 << ARCH_ARM64)) {
        /* Prepend ARM64 entry stub to the ARM64 assembly */
        char arm64_stub[1024];
        size_t stub_len = universal_trampoline_arm64(arm64_stub, sizeof(arm64_stub), "_start_arm64_main");
        char *arm64_full = (char *)malloc(stub_len + builder->asm_arm64_len + 1);
        if (!arm64_full) return -1;
        memcpy(arm64_full, arm64_stub, stub_len);
        memcpy(arm64_full + stub_len, builder->asm_arm64, builder->asm_arm64_len);
        arm64_full[stub_len + builder->asm_arm64_len] = '\0';

        snprintf(obj_files[arch_count], sizeof(obj_files[arch_count]),
                 "/tmp/aether_universal_arm64.o");
        char asm_path[1024];
        snprintf(asm_path, sizeof(asm_path), "/tmp/aether_universal_arm64.asm");
        FILE *f = fopen(asm_path, "w");
        if (f) { fwrite(arm64_full, 1, stub_len + builder->asm_arm64_len, f); fclose(f); }
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "%s -o %s %s 2>/dev/null", builder->config.arm64_assembler, obj_files[arch_count], asm_path);
        int ret = system(cmd);
        if (ret != 0) {
            fprintf(stderr, "Warning: ARM64 assembly failed (exit %d). Install aarch64-linux-gnu-as or use --target asm-arm64\n", ret);
            fprintf(stderr, "  Universal binary will contain x86_64 only\n");
            free(arm64_full);
            remove(asm_path);
        } else {
            remove(asm_path);
            free(arm64_full);
            archs[arch_count] = ARCH_ARM64;
            actual_flags |= (1 << ARCH_ARM64);
            arch_count++;
        }
    }

    if (builder->config.arch_flags & (1 << ARCH_RISCV64)) {
        /* Prepend RISC-V entry stub to the RISC-V assembly */
        char rv_stub[1024];
        size_t stub_len = universal_trampoline_riscv64(rv_stub, sizeof(rv_stub), "_start_riscv64_main");
        char *rv_full = (char *)malloc(stub_len + builder->asm_riscv64_len + 1);
        if (!rv_full) return -1;
        memcpy(rv_full, rv_stub, stub_len);
        memcpy(rv_full + stub_len, builder->asm_riscv64, builder->asm_riscv64_len);
        rv_full[stub_len + builder->asm_riscv64_len] = '\0';

        snprintf(obj_files[arch_count], sizeof(obj_files[arch_count]),
                 "/tmp/aether_universal_riscv64.o");
        char asm_path[1024];
        snprintf(asm_path, sizeof(asm_path), "/tmp/aether_universal_riscv64.asm");
        FILE *f = fopen(asm_path, "w");
        if (f) { fwrite(rv_full, 1, stub_len + builder->asm_riscv64_len, f); fclose(f); }
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "%s -o %s %s 2>/dev/null", builder->config.riscv64_assembler, obj_files[arch_count], asm_path);
        int ret = system(cmd);
        if (ret != 0) {
            fprintf(stderr, "Warning: RISC-V assembly failed (exit %d). Install riscv64-linux-gnu-as or use --target asm-riscv64\n", ret);
            fprintf(stderr, "  Universal binary will contain x86_64 only\n");
            free(rv_full);
            remove(asm_path);
        } else {
            remove(asm_path);
            free(rv_full);
            archs[arch_count] = ARCH_RISCV64;
            actual_flags |= (1 << ARCH_RISCV64);
            arch_count++;
        }
    }

    if (arch_count == 0) {
        fprintf(stderr, "Error: no architectures selected\n");
        return -1;
    }

    /* Step 3: Regenerate trampoline with only the architectures that succeeded */
    {
        char tramp_buf[4096];
        int has_arm64 = actual_flags & (1 << ARCH_ARM64);
        int has_riscv64 = actual_flags & (1 << ARCH_RISCV64);
        size_t tn = snprintf(tramp_buf, sizeof(tramp_buf),
            "; CPU detection trampoline\n"
            "section .text.trampoline\n"
            "extern _aether_entry\n"
            "%s"
            "%s"
            "global _start\n"
            "_start:\n"
            "    pushfq\n"
            "    pushfq\n"
            "    xor qword [rsp], 0x00200000\n"
            "    popfq\n"
            "    pushfq\n"
            "    pop rax\n"
            "    xor rax, [rsp]\n"
            "    and rax, 0x00200000\n"
            "    popfq\n"
            "    test rax, rax\n"
            "    jnz _start_x86\n"
            "    jmp %s\n"
            "_start_x86:\n"
            "    jmp _aether_entry\n",
            has_arm64 ? "extern _start_arm64\n" : "",
            has_riscv64 ? "extern _start_riscv64\n" : "",
            has_arm64 ? "_start_arm64" : (has_riscv64 ? "_start_riscv64" : "_aether_entry"));
        if (tn < 0 || (size_t)tn >= sizeof(tramp_buf)) return -1;
        memcpy(builder->trampoline_asm, tramp_buf, (size_t)tn);
        builder->trampoline_len = (size_t)tn;
    }

    /* Step 4: Write and assemble the trampoline */
    char trampoline_obj[1024];
    snprintf(trampoline_obj, sizeof(trampoline_obj), "/tmp/aether_trampoline.o");
    {
        char asm_path[1024];
        snprintf(asm_path, sizeof(asm_path), "/tmp/aether_trampoline.asm");
        FILE *f = fopen(asm_path, "w");
        if (f) { fwrite(builder->trampoline_asm, 1, builder->trampoline_len, f); fclose(f); }
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "nasm -f elf64 -o %s %s", trampoline_obj, asm_path);
        int ret = system(cmd);
        if (ret != 0) {
            fprintf(stderr, "Error: trampoline assembly failed (exit %d)\n", ret);
            return -1;
        }
        remove(asm_path);
    }

    /* Step 4: Link all object files together */
    char cmd[8192];
    int pos = snprintf(cmd, sizeof(cmd), "%s -o %s -T /tmp/aether_universal.ld %s",
                       builder->config.linker, output_file, trampoline_obj);

    for (int i = 0; i < arch_count; i++) {
        pos += snprintf(cmd + pos, sizeof(cmd) - pos, " %s", obj_files[i]);
    }

    /* Write linker script that merges sections */
    FILE *ldf = fopen("/tmp/aether_universal.ld", "w");
    if (ldf) {
        fprintf(ldf, "OUTPUT_FORMAT(elf64-x86-64)\n");
        fprintf(ldf, "ENTRY(_start)\n");
        fprintf(ldf, "SECTIONS {\n");
        fprintf(ldf, "  . = 0x400000;\n");
        fprintf(ldf, "  .text.trampoline : { *(.text.trampoline) }\n");
        fprintf(ldf, "  .text.x86_64 : { *(.text.x86_64) *(.text) }\n");
        fprintf(ldf, "  .text.arm64 : { *(.text.arm64) }\n");
        fprintf(ldf, "  .text.riscv64 : { *(.text.riscv64) }\n");
        fprintf(ldf, "  .rodata : { *(.rodata) *(.rodata.*) }\n");
        fprintf(ldf, "  .data : { *(.data) *(.data.*) }\n");
        fprintf(ldf, "  .bss : { *(.bss) *(.bss.*) *(COMMON) }\n");
        fprintf(ldf, "}\n");
        fclose(ldf);
    }

    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Error: linking universal binary failed (exit %d)\n", ret);
        return -1;
    }

    /* Step 5: Cleanup temp files */
    remove(trampoline_obj);
    for (int i = 0; i < arch_count; i++) {
        remove(obj_files[i]);
    }
    remove("/tmp/aether_universal.ld");

    if (builder->config.verbose) {
        printf("Universal binary created: %s\n", output_file);
        printf("  Architectures: ");
        for (int i = 0; i < arch_count; i++) {
            printf("%s%s", i > 0 ? ", " : "", arch_name(archs[i]));
        }
        printf("\n");
    }

    return 0;
}
