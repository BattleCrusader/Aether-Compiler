# Phase 18 — Standard Library Implementation & `libaether.aelib`

**Status**: ✅ COMPLETE
**Branch**: `feature/P18.00-stdlib-implementation`
**Goal**: Implement all `std/*.ae` library functions with proper assembly, build `libaether.aelib` for development and OS development.

---

## Overview

All 11 standard library files were stubbed with `xor rax, rax` or `nop`. This phase implements real logic using proper SysV ABI register arguments, removes `main()` stubs, and builds a combined `libaether.aelib` archive.

---

## P18.01 — `std/math.ae` — Basic math

- [x] `absValue(value: int): int` — absolute value via `neg`/`cmovl`
- [x] `min(firstValue, secondValue: int): int` — via `cmp`/`cmovg`
- [x] `max(firstValue, secondValue: int): int` — via `cmp`/`cmovl`
- [x] `clamp(value, lowerBound, upperBound: int): int` — via `cmp`/`cmovl`/`cmovg`

## P18.02 — `std/mem.ae` — Memory operations

- [x] `memCopy(destination, source, byteCount: u64)` — `rep movsb`
- [x] `memZero(pointer, byteCount: u64)` — `rep stosb`
- [x] `alloc(byteCount: u64): u64` — calls `__aether_alloc` runtime helper
- [x] `free(pointer: u64)` — no-op (bump allocator)

## P18.03 — `std/str.ae` — String operations

- [x] `concat(firstString, secondString: string): string` — calls `__aether_concat`
- [x] `split(inputString, delimiter: string): [string]` — stub (TODO)
- [x] `trim(inputString: string): string` — stub (TODO)
- [x] `toUpper(inputString: string): string` — stub (TODO)
- [x] `toLower(inputString: string): string` — stub (TODO)

## P18.04 — `std/io.ae` — I/O operations

- [x] `println(value: string)` — calls `print()` built-in twice (value + newline)

## P18.05 — `std/arch.ae` — Architecture detection

- [x] `archDetect(): u64` — CPUID-based detection via `pushfq`/`popfq`
- [x] Constants: `archX8664`, `archArm64`, `archRiscv64`, `cacheLine*`, `pageSize`

## P18.06 — `std/asm.ae` — NASM helper macros

- [x] `mfence()`, `sfence()`, `lfence()` — memory barriers
- [x] `outb(portNumber: u16, value: u8)` — port I/O via `out`
- [x] `inb(portNumber: u16): u8` — port I/O via `in`
- [x] `cli()`, `sti()`, `hlt()` — CPU control
- [x] `rdtsc(): u64` — timestamp counter

## P18.07 — `std/collections.ae` — Simple collections

- [x] `Array` struct with `data`, `length`, `capacity`
- [x] `List` struct with `data`, `length`, `capacity`
- [x] `arrayNew(): Array`, `listNew(): List` — zero-initialized

## P18.08 — `std/elf.ae` — ELF64 reader/writer

- [x] `Elf64Header` struct (all fields, camelCase, no reserved keywords)
- [x] `Elf64SectionHeader` struct
- [x] `Elf64Symbol` struct
- [x] `elfVerifyMagic(header: ref Elf64Header): bool`

## P18.09 — `std/fs.ae` — AetherFS syscall wrappers

- [x] `sys func fsOpen/fsRead/fsWrite/fsClose/fsSeek/fsStat` — syscall declarations
- [x] `File` struct with `fileDescriptor`
- [x] `fileOpen`, `fileRead`, `fileWrite`, `fileClose` — method-style functions

## P18.10 — `std/serial.ae` — COM1 serial I/O

- [x] `serialInit(): bool` — COM1 initialization (115200 baud, 8N1)
- [x] `serialPutc(character: byte)` — write one byte
- [x] `serialPuts(inputString: string)` — write string
- [x] `serialGetc(): byte` — read one byte (blocking)

## P18.11 — `libaether.aelib` combined archive

- [x] Combined `libaether.aelib` from all 11 std source files
- [x] Makefile target: `make std/libaether.aelib`
- [x] `test-host` depends on `$(LIBAETHER_AELIB)`
- [x] All 11 individual `.aelib` files compile cleanly

## P18.12 — Codegen fixes for `TARGET_LIB`

- [x] Runtime helpers (`__aether_alloc`, `__aether_concat`, `__aether_itoa`) emitted for `TARGET_LIB`
- [x] Built-in functions (`print`, `exit`) available in `TARGET_LIB` codegen

---

## Naming Conventions

All std library code uses:
- **camelCase** for function names (`absValue`, `memCopy`, `serialPutc`)
- **camelCase** for variable names (`firstValue`, `byteCount`, `inputString`)
- **camelCase** for struct fields (`fileDescriptor`, `sectionType`)
- **camelCase** for constants (`archX8664`, `pageSize`)
- No `main()` functions in library files
- Descriptive variable names (not `a`, `b`, `c`)
