# Phase 10 — Universal Binary & Multi-Arch Dispatch

**Goal**: Compile a single Aether source file into a universal binary that runs natively on multiple architectures (x86_64, ARM64, RISC-V) without an interpreter, JIT, or emulation layer. The binary contains architecture-specific code slices and a tiny CPU detection trampoline.

**Branch**: `feature/P10.00-universal-binary`

---

## P10.01 — Fat Binary Container Format 🟢 DONE

- [x] Define `MultiArchHeader` struct in `include/aether/universal.h`:
  - Magic number, version, architecture count
  - Per-arch entry: arch ID, offset, size, entry point offset
- [x] Define `ArchId` enum: `ARCH_X86_64`, `ARCH_ARM64`, `ARCH_RISCV64`
- [x] Define the `.note.aether_multiarch` ELF note section format
- [x] Define the CPU detection trampoline layout (shared entry point)
- [x] Unit tests for header construction and parsing

## P10.02 — CPU Detection Trampoline 🟢 DONE

- [x] x86_64 detection via CPUID (EFLAGS.ID bit test)
- [x] ARM64 detection via `_start_arm64` entry point stub
- [x] RISC-V detection via `_start_riscv64` entry point stub
- [x] Trampoline emits `jmp` to the correct architecture's entry point
- [x] Trampoline fits in ~64 bytes (fits in any cache line)
- [x] Unit tests: trampoline assembled correctly for each arch

## P10.03 — Dual Compilation Pipeline 🟢 DONE

- [x] `codegen_generate_universal()` — runs codegen for each target architecture
- [x] Each arch gets its own NASM output → assembled to object file
- [x] Object files merged into single ELF with multi-arch sections
- [x] Shared sections (.rodata, .data, .bss) emitted once, shared by all arches
- [x] Entry point replaced with CPU detection trampoline
- [x] Integration with existing `codegen_assemble()` pipeline

## P10.04 — ARM64/RISC-V ELF64 Assembler Integration 🟢 DONE

- [x] Detect `aarch64-linux-gnu-as` on the system
- [x] Detect `riscv64-linux-gnu-as` on the system
- [x] Fallback: graceful warning when cross-assembler not found
- [x] ARM64 assembly → ARM64 ELF64 object file
- [x] RISC-V assembly → RISC-V ELF64 object file
- [x] Error message when cross-assembler not found

## P10.05 — `--target universal` CLI Flag 🟢 DONE

- [x] `--target universal` — compile for x86_64 + ARM64
- [x] `--target universal-all` — compile for x86_64 + ARM64 + RISC-V
- [x] `--target universal-x86_64-arm64` — explicit arch list
- [x] Default output name for universal binaries
- [x] `aether.toml` `[build]` target = "universal" support
- [x] Integration tests: compile with `--target universal`, verify output

## P10.06 — Shared Section Deduplication 🟢 DONE

- [x] String literals deduplicated across architectures
- [x] Const data deduplicated
- [x] `.rodata` section shared by all arch slices
- [x] `.data` section shared (writable, but same initial values)
- [x] `.bss` section shared (zero-initialized, same layout)
- [x] Verification: universal binary is smaller than 2× single-arch binary

## P10.07 — Architecture-Specific Init 🟢 DONE

- [x] `#if arch() == "x86_64"` / `#elif arch() == "arm64"` compile-time guards
- [x] `@arch(x86_64)` / `@arch(arm64)` function attributes — only included in that arch's slice
- [x] `@arch_shared` attribute — included in all slices (default)
- [x] Compiler error when arch-specific code references symbols not present in that slice
- [x] Test: kernel entry with arch-specific GDT vs system register setup

## P10.08 — Multi-Arch Test Suite 🟢 DONE

- [x] Test fixture: simple arithmetic (add, sub, mul) — verify all 3 archs
- [x] Test fixture: memory operations (load, store) — verify all 3 archs
- [x] Test fixture: control flow (if, while, for) — verify all 3 archs
- [x] Test fixture: function calls with SysV ABI — verify x86_64 + ARM64
- [x] Test fixture: inline NASM with arch-specific asm blocks
- [x] Test fixture: full universal binary build (compile + merge)
- [x] `make test-universal` target

## P10.09 — Cross-Compilation Toolchain Detection 🟢 DONE

- [x] Auto-detect `aarch64-linux-gnu-as` on PATH
- [x] Auto-detect `riscv64-linux-gnu-as` on PATH
- [x] Fallback to NASM + multi-target backend when cross-assembler not found
- [x] `--toolchain-prefix aarch64-linux-gnu-` CLI flag
- [x] `aether.toml` `[toolchain]` section for cross-compiler paths
- [x] Clear error messages when toolchain is missing

## P10.10 — Integration with `aether.toml` for Multi-Arch Builds 🟢 DONE

- [x] `[build]` `targets = ["x86_64", "arm64"]` — list of architectures
- [x] `[build]` `universal = true` — shorthand for x86_64 + ARM64
- [x] `[toolchain]` section for cross-assembler paths
- [x] `aether build` reads multi-arch config from `aether.toml`
- [x] Integration test: `aether.toml` with universal config

---

## Legend

| Status | Meaning |
|--------|---------|
| 🟢 DONE | Completed and verified |
| 🔵 IN PROGRESS | Currently being worked on |
| 🟡 HOLD | Blocked, waiting on something else |
| 🔴 NOT STARTED | Planned but not started |
| ⚪ CANCELLED | No longer planned |
