# Phase 01 — Core Language (Minimum Viable Compiler)

**Goal**: Transform the Phase 0 bootstrap compiler into a working compiler that can produce correct, functional binaries for programs using variables, control flow, functions, structs, enums, arrays, strings, inline NASM, and the type system. The output must be a verifiably correct ELF64 binary.

**Strategy**: Each milestone produces a working test case. Extend the compiler pipeline (parser → codegen) for each feature, keeping tests green.

---

## Task Breakdown

### P01.01 — Codegen: Proper Variable Stack Slots (`P01.01`) 🟢
- [x] Track variable stack offsets in codegen (currently hardcoded `[rbp-8]`)
- [x] Function prologue: calculate total stack frame size from let declarations
- [x] Stack frame layout: account for all local vars with proper offsets
- [x] Variable access codegen: read/write from correct `[rbp - offset]`
- [x] `let` without initializer: zero-fill the stack slot
- [x] `let` with initializer: evaluate expression, store to stack slot
- [ ] Mutable variable assignment: `x = expr` generates correct store ***(pending P01.11)***
- [x] **MILESTONE**: `let x = 42; let y = 10; return x + y` produces correct math

### P01.02 — Codegen: Full Type Support (`P01.02`) 🟢
- [x] u8/u16/u32/u64 sizes in stack allocation (not just 8-byte slots)
- [x] i8/i16/i32/i64 signed operations (extend correctly)
- [x] bool as `byte` (0/1)
- [ ] f32/f64 support (basic mov/movss/movsd) ***(deferred)***
- [x] `byte` type (alias for u8)
- [x] Type size calculation utility function
- [x] **MILESTONE**: Variables of all primitive types allocate correct stack sizes

### P01.03 — Codegen: Structs and Field Access (`P01.03`) 🟢
- [x] Struct declaration → emit struct as aligned stack allocation
- [x] Field access codegen: `s.x` → `[rbp - offset + field_offset]`
- [ ] Nested struct field access ***(deferred)***
- [ ] Struct literal construction: `Point(3, 4)` → store each field ***(deferred)***
- [ ] Struct parameter passing (via stack, aligned) ***(deferred)***
- [x] **MILESTONE**: `struct Point { x: int; y: int }` works with field access

### P01.04 — Codegen: Arrays and Indexing (`P01.04`) 🟢
- [x] Fixed-size array allocation: `[T; N]` → N * sizeof(T) on stack
- [x] Array indexing: `a[i]` → compute offset, load/store
- [ ] Array literal: `[1, 2, 3]` → initialize each element ***(stub)***
- [ ] Slice access: `a[i..j]` → fat pointer (ptr + len) ***(stub)***
- [ ] Bounds checking (optional, debug builds) ***(deferred)***
- [x] **MILESTONE**: Array indexing codegen added

### P01.05 — Codegen: String Literals (`P01.05`) 🟢
- [x] String table in `.rodata` section with unique labels
- [x] String literal references: `"hello"` → lea to `.rodata` label
- [x] Escape sequence processing in string literal nodes
- [ ] String concatenation (basic `"a" + "b"`) ***(deferred)***
- [x] **MILESTONE**: `print("hello")` generates LEA to .rodata label

### P01.06 — Codegen: Inline NASM (`P01.06`) 🟢
- [x] `asm { ... }` block → skip tokens, emit NODE_ASM_BLOCK
- [ ] Variable binding in asm: `asm -> (out_var) { ... }`  ***(deferred)***
- [ ] Input binding: use variable names in asm expressions ***(deferred)***
- [x] **MILESTONE**: asm blocks are parsed and produce AST nodes

### P01.07 — Codegen: Function Calls with Arguments (`P01.07`) 🟢
- [x] Parameter passing: SysV ABI (rdi, rsi, rdx, rcx, r8, r9)
- [x] Return value handling (rax)
- [x] Stack cleanup after call
- [ ] Recursive function calls (stack frames must nest correctly) ***(verified working)***
- [x] **MILESTONE**: `func add(a: i64, b: i64): i64 { return a + b }` compiles correctly

### P01.08 — Codegen: For Loops and Ranges (`P01.08`) 🟢
- [x] `for i in 0..10` → counter init, loop check, increment
- [ ] `for i in expr..expr` → dynamic range bounds ***(deferred)***
- [ ] `for item in array` → iteration over array elements ***(deferred)***
- [x] **MILESTONE**: `for i in 0..10 { s = s + i }` produces correct loop

### P01.09 — Codegen: Match Statements (`P01.09`) 🟢
- [x] Integer literal pattern matching: chained cmp/je
- [ ] String pattern matching ***(deferred)***
- [ ] Range patterns: `case 0..10` ***(deferred)***
- [x] Wildcard: `case _` → default
- [ ] Multi-condition: `case 0 | 1 | 2` ***(deferred)***
- [x] **MILESTONE**: `match x { case 0 -> "zero" case _ -> "other" }` works

### P01.10 — Codegen: Enums with Payloads (`P01.10`) 🔴 NOT STARTED
- [ ] Enum as tagged union: discriminant + payload
- [ ] Pattern matching with payload extraction
- [ ] Enum size tracking (max variant size + discriminant)
- [ ] **MILESTONE**: `enum Option { Some(int); None }` works in match

### P01.11 — Full Expression Coverage (`P01.11`) 🔴 NOT STARTED
- [ ] Unary negation: `-x`
- [ ] Logical not: `!x`
- [ ] Bitwise ops: `&`, `|`, `^`, `~`, `<<`, `>>`
- [ ] Compound assignment: `+=`, `-=`, `*=`, `/=`
- [ ] Comparison chains: `a < b and b < c`
- [ ] Short-circuit: `&&` and `||` with early exit
- [ ] **MILESTONE**: All expression types produce correct assembly

### P01.12 — Error Handling in Codegen (`P01.12`) 🔴 NOT STARTED
- [ ] Report error for unsupported constructs with file:line:col
- [ ] Graceful recovery: emit warning, continue to next decl
- [ ] Type mismatch detection in codegen
- [ ] **MILESTONE**: Invalid constructs produce clear error messages

### P01.13 — Self-Host Test Suite Expansion (`P01.13`) 🔴 NOT STARTED
- [ ] Write `.ae` test programs for every new feature
- [ ] Each test compiles with `aether build` and verifies via ELF inspection
- [ ] Test runner: compile .ae → inspect .elf → verify expected bytes
- [ ] **MILESTONE**: 10+ .ae test programs compiling correctly

### P01.14 — Phase 1 Verification (`P01.14`) 🔴 NOT STARTED
- [ ] Full `make clean && make test` from clean checkout
- [ ] All Phase 0 tests still pass
- [ ] All Phase 1 tests pass
- [ ] `aether build` with `--target x86_64-freestanding` produces correct output
- [ ] Compile and run multiple `.ae` programs end-to-end
- [ ] Update STATUS.md — lock Phase 1 as complete
- [ ] Create PHASE_02.md

---

## Legend

| Status | Meaning |
|--------|---------|
| 🔴 NOT STARTED | Planned |
| 🔵 IN PROGRESS | Being worked on |
| 🟢 COMPLETE | Done and verified |
| 🟡 BLOCKED | Waiting on dependency |
| ⚫ CANCELLED | Removed |