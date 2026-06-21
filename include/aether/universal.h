#ifndef AETHER_UNIVERSAL_H
#define AETHER_UNIVERSAL_H

#include <stdint.h>
#include <stddef.h>

/* ================================================================
 * Aether Universal Binary — Multi-Arch Container Format
 *
 * A universal binary is a single ELF64 file containing native code
 * for multiple architectures. A tiny CPU detection trampoline at
 * the ELF entry point detects the running CPU and dispatches to
 * the correct architecture's code.
 *
 * Container format:
 *   ELF64 header (entry point → CPU detection trampoline)
 *   .note.aether_multiarch  — architecture table (MultiArchNote)
 *   .text.trampoline        — CPU detection + dispatch code
 *   .text.x86_64            — x86_64 native code
 *   .text.arm64             — ARM64 native code
 *   .text.riscv64           — RISC-V native code
 *   .rodata                 — shared read-only data
 *   .data / .bss            — shared writable/zero data
 * ================================================================ */

/* --- Architecture identifiers --- */
typedef enum {
    ARCH_X86_64 = 0,
    ARCH_ARM64  = 1,
    ARCH_RISCV64 = 2,
    ARCH_COUNT
} ArchId;

/* --- Architecture entry in the multi-arch table --- */
typedef struct {
    uint32_t arch_id;       /* ArchId enum value */
    uint32_t text_offset;   /* byte offset from ELF base to this arch's .text */
    uint32_t text_size;     /* size of this arch's .text in bytes */
    uint32_t entry_offset;  /* byte offset from text_offset to entry point */
    uint32_t rodata_offset; /* byte offset from ELF base to shared .rodata */
    uint32_t rodata_size;   /* size of shared .rodata */
    uint32_t data_offset;   /* byte offset from ELF base to shared .data */
    uint32_t data_size;     /* size of shared .data */
    uint32_t bss_offset;    /* byte offset from ELF base to .bss */
    uint32_t bss_size;      /* size of .bss */
    uint32_t reserved[6];   /* future use */
} __attribute__((packed)) ArchEntry;

/* --- Multi-arch note section header (in .note.aether_multiarch) --- */
typedef struct {
    uint32_t namesz;        /* sizeof("Aether") = 7 */
    uint32_t descsz;        /* sizeof(MultiArchDescriptor) */
    uint32_t type;          /* NT_AETHER_MULTIARCH = 0xAE7E0001 */
    char     name[8];       /* "Aether\0\0" */
} __attribute__((packed)) MultiArchNoteHeader;

/* --- Multi-arch descriptor (contents of the note) --- */
typedef struct {
    uint32_t version;       /* format version (1) */
    uint32_t arch_count;    /* number of architectures in this binary */
    ArchEntry entries[ARCH_COUNT]; /* one per architecture */
} __attribute__((packed)) MultiArchDescriptor;

/* --- Note type constant --- */
#define NT_AETHER_MULTIARCH 0xAE7E0001

/* --- Universal binary build configuration --- */
typedef struct {
    int arch_flags;         /* bitmask: (1 << ARCH_X86_64) | (1 << ARCH_ARM64) | ... */
    const char *output_file;
    const char *x86_64_assembler;   /* e.g., "nasm" */
    const char *arm64_assembler;    /* e.g., "aarch64-linux-gnu-as" */
    const char *riscv64_assembler;  /* e.g., "riscv64-linux-gnu-as" */
    const char *linker;             /* e.g., "ld" or "x86_64-elf-ld" */
    int verbose;
} UniversalConfig;

/* --- Universal binary builder state --- */
typedef struct {
    UniversalConfig config;
    /* Per-arch assembly text buffers */
    char *asm_x86_64;
    size_t asm_x86_64_len;
    char *asm_arm64;
    size_t asm_arm64_len;
    char *asm_riscv64;
    size_t asm_riscv64_len;
    /* Per-arch object file paths (temp) */
    char obj_x86_64[1024];
    char obj_arm64[1024];
    char obj_riscv64[1024];
    /* Trampoline assembly text */
    char trampoline_asm[4096];
    size_t trampoline_len;
} UniversalBuilder;

/* --- API --- */

/* Initialize a universal build config with defaults */
void universal_config_init(UniversalConfig *config);

/* Set which architectures to include */
void universal_config_set_archs(UniversalConfig *config, int arch_mask);

/* Create a universal builder */
UniversalBuilder *universal_builder_create(const UniversalConfig *config);
void universal_builder_destroy(UniversalBuilder *builder);

/* Add per-architecture assembly text to the builder */
int universal_builder_add_asm(UniversalBuilder *builder, ArchId arch,
                               const char *asm_text, size_t asm_len);

/* Generate the CPU detection trampoline assembly */
int universal_generate_trampoline(UniversalBuilder *builder);

/* Build the universal binary: assemble each arch, merge, write output */
int universal_build(UniversalBuilder *builder, const char *output_file);

/* --- Trampoline generation --- */
/* Generate x86_64 CPUID-based detection asm */
size_t universal_trampoline_x86_64(char *buf, size_t buf_size,
                                    const char *arm64_entry_label);

/* Generate ARM64 detection asm (reads MIDR_EL1) */
size_t universal_trampoline_arm64(char *buf, size_t buf_size,
                                    const char *arm64_entry_label);

/* Generate RISC-V detection asm (reads mvendorid) */
size_t universal_trampoline_riscv64(char *buf, size_t buf_size,
                                      const char *riscv64_entry_label);

/* --- Helpers --- */
const char *arch_name(ArchId arch);
const char *arch_asm_section_name(ArchId arch);
int arch_asm_flags(ArchId arch); /* returns (1 << arch) */

#endif /* AETHER_UNIVERSAL_H */
