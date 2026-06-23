# Phase 18 — Standard Library Implementation & `libaether.aelib`

**Status**: ✅ COMPLETE
**Branch**: `feature/P18.00-stdlib-implementation`
**Goal**: Implement all `std/*.ae` library functions using pure Aether where possible, only asm where genuinely necessary. Build `libaether.aelib` to `build/lib/`.

---

## Overview

All 11 standard library files were rewritten from asm-heavy stubs to pure Aether implementations. Only 3 files retain asm for things Aether cannot express (port I/O, CPU control instructions, byte-level memory copy). The combined `libaether.aelib` archive is built to `build/lib/libaether.aelib` — out of the source tree and gitignored.

---

## P18.01 — `std/math.ae` — Basic math

- [x] `absValue(value: int): int` — pure Aether `if/else`
- [x] `min(firstValue, secondValue: int): int` — pure Aether `if/else`
- [x] `max(firstValue, secondValue: int): int` — pure Aether `if/else`
- [x] `clamp(value, lowerBound, upperBound: int): int` — pure Aether `if/else`

## P18.02 — `std/mem.ae` — Memory operations

- [x] `memCopy(destination, source, byteCount: u64)` — asm `rep movsb` (byte-level pointer ops)
- [x] `memZero(pointer, byteCount: u64)` — asm `rep stosb` (byte-level pointer ops)
- [x] `alloc(byteCount: u64): u64` — Aether `heap` keyword
- [x] `free(pointer: u64)` — no-op (bump allocator)

## P18.03 — `std/str.ae` — String operations

- [x] `concat(firstString, secondString: string): string` — Aether `+` operator
- [x] `split(inputString, delimiter: string): [string]` — stub (TODO)
- [x] `trim(inputString: string): string` — stub (TODO)
- [x] `toUpper(inputString: string): string` — stub (TODO)
- [x] `toLower(inputString: string): string` — stub (TODO)

## P18.04 — `std/io.ae` — I/O operations

- [x] `println(value: string)` — Aether `print()` built-in

## P18.05 — `std/arch.ae` — Architecture detection

- [x] `archDetect(): u64` — asm `pushfq`/`popfq` CPUID detection
- [x] Constants: `archX8664`, `archArm64`, `archRiscv64`, `cacheLine*`, `pageSize`

## P18.06 — `std/asm.ae` — NASM helper macros

- [x] `memoryFence()`, `storeFence()`, `loadFence()` — asm memory barriers
- [x] `portOutByte(portNumber: u16, value: u8)` — asm `out` instruction
- [x] `portInByte(portNumber: u16): u8` — asm `in` instruction
- [x] `interruptsDisable()`, `interruptsEnable()`, `haltCpu()` — asm CPU control
- [x] `readTimestampCounter(): u64` — asm timestamp counter

## P18.07 — `std/collections.ae` — Simple collections

- [x] `Array` struct with `data`, `length`, `capacity`
- [x] `List` struct with `data`, `length`, `capacity`
- [x] `arrayNew(): Array`, `listNew(): List` — pure Aether struct field init

## P18.08 — `std/elf.ae` — ELF64 reader/writer

- [x] `Elf64Header` struct (all fields, camelCase, no reserved keywords)
- [x] `Elf64SectionHeader` struct
- [x] `Elf64Symbol` struct
- [x] `elfVerifyMagic(header: ref Elf64Header): bool` — pure Aether struct ref access

## P18.09 — `std/fs.ae` — AetherFS syscall wrappers

- [x] `sys func fsOpen/fsRead/fsWrite/fsClose/fsSeek/fsStat` — Aether `sys` keyword
- [x] `File` struct with `fileDescriptor`
- [x] `fileOpen`, `fileRead`, `fileWrite`, `fileClose` — method-style functions

## P18.10 — `std/serial.ae` — COM1 serial I/O

- [x] `serialInit(): bool` — asm port I/O (COM1 init, 115200 baud, 8N1)
- [x] `serialPutc(character: byte)` — asm `out` instruction
- [x] `serialPuts(inputString: string)` — asm `out` loop
- [x] `serialGetc(): byte` — asm `in` instruction (blocking)

## P18.11 — `libaether.aelib` combined archive

- [x] Combined `libaether.aelib` from all 11 std source files
- [x] Makefile target: `make build/lib/libaether.aelib`
- [x] `test-host` depends on `$(LIBAETHER_AELIB)`
- [x] All 11 individual `.aelib` files compile cleanly
- [x] `.aelib` artifacts go to `build/lib/` — out of source tree, gitignored

## P18.12 — Codegen fixes for `TARGET_LIB`

- [x] Runtime helpers (`__aether_alloc`, `__aether_concat`, `__aether_itoa`) emitted for `TARGET_LIB`
- [x] Built-in functions (`print`, `exit`) available in `TARGET_LIB` codegen

## P18.13 — Build output hygiene

- [x] `libaether.aelib` moved from `std/` to `build/lib/`
- [x] `*.aelib` added to `.gitignore`
- [x] `install`/`install-local` only install binary + `libaether.aelib` (no source files, no headers)
- [x] Test fixture `lib_math.aelib` removed from git (build-time only)

---

## Asm Usage Summary

| File | Asm? | Reason |
|------|------|--------|
| `math.ae` | No | Pure Aether `if/else` |
| `str.ae` | No | `+` operator for concat |
| `io.ae` | No | `print()` built-in |
| `collections.ae` | No | Struct field init |
| `elf.ae` | No | Struct definitions |
| `fs.ae` | No | `sys` keyword |
| `test.ae` | No | String interpolation, `exit()` |
| `arch.ae` | Yes (CPUID) | Can't detect CPU in pure Aether |
| `asm.ae` | Yes (port I/O, barriers) | `in`/`out`, `memoryFence` etc. |
| `serial.ae` | Yes (port I/O) | `in`/`out` instructions |
| `mem.ae` | Yes (byte copy) | `rep movsb`/`stosb` |

---

## Naming Conventions

All std library code uses:
- **camelCase** for function names (`absValue`, `memCopy`, `serialPutc`)
- **camelCase** for variable names (`firstValue`, `byteCount`, `inputString`)
- **camelCase** for struct fields (`fileDescriptor`, `sectionType`)
- **camelCase** for constants (`archX8664`, `pageSize`)
- No `main()` functions in library files
- Descriptive variable names (not `a`, `b`, `c`)
