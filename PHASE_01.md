# Phase 01 — Core Language (Minimum Viable Compiler)

**Goal**: Transform the Phase 0 bootstrap compiler into a working compiler that can produce correct, functional binaries for programs using variables, control flow, functions, structs, enums, arrays, strings, inline NASM, and the type system. The output must be a verifiably correct ELF64 binary.

**Strategy**: Each milestone produces a working test case. Extend the compiler pipeline (parser → codegen) for each feature, keeping tests green.

---

## Task Breakdown

### P01.01 — Codegen: Proper Variable Stack Slots (`P01.01`)
- [ ] Track variable stack offsets in codegen (currently hardcoded `[rbp-8]`)
- [ ] Function prologue: calculate total stack frame size from let declarations
- [ ] Stack frame layout: account for all local vars with proper offsets
- [ ] Variable access codegen: read/write from correct `[rbp - offset]`
- [ ] `let` without initializer: zero-fill the stack slot
- [ ] `let` with initializer: evaluate expression, store to stack slot
- [ ] Mutable variable assignment: `x = expr` generates correct store
- [ ] **MILESTONE**: `let x = 42; let y = 10; return x + y` produces correct math

### P01.02 — Codegen: Full Type Support (`P01.02`)
- [ ] u8/u16/u32/u64 sizes in stack allocation (not just 8-byte slots)
- [ ] i8/i16/i32/i64 signed operations (extend correctly)
- [ ] bool as `byte` (0/1)
- [ ] f32/f64 support (basic mov/movss/movsd)
- [ ] `byte` type (alias for u8)
- [ ] Type size calculation utility function
- [ ] **MILESTONE**: Variables of all primitive types allocate correct stack sizes

### P01.03 — Codegen: Structs and Field Access (`P01.03`)
- [ ] Struct declaration → emit struct as aligned stack allocation
- [ ] Field access codegen: `s.x` → `[rbp - offset + field_offset]`
- [ ] Nested struct field access
- [ ] Struct literal construction: `Point(3, 4)` → store each field
- [ ] Struct parameter passing (via stack, aligned)
- [ ] **MILESTONE**: `struct Point { x: int; y: int }` works with field access

### P01.04 — Codegen: Arrays and Indexing (`P01.04`)
- [ ] Fixed-size array allocation: `[T; N]` → N * sizeof(T) on stack
- [ ] Array indexing: `a[i]` → compute offset, load/store
- [ ] Array literal: `[1, 2, 3]` → initialize each element
- [ ] Slice access: `a[i..j]` → fat pointer (ptr + len)
- [ ] Bounds checking (optional, debug builds)
- [ ] **MILESTONE**: Array initialization and indexing works

### P01.05 — Codegen: String Literals (`P01.05`)
- [ ] String table in `.rodata` section with unique labels
- [ ] String literal references: `"hello"` → lea to `.rodata` label
- [ ] Escape sequence processing in string literal nodes
- [ ] String concatenation (basic `"a" + "b"`)
- [ ] **MILESTONE**: `print("hello")` generates LEA to .rodata label

### P01.06 — Codegen: Inline NASM (`P01.06`)
- [ ] `asm { ... }` block → passthrough emit to the NASM output
- [ ] Variable binding in asm: `asm -> (out_var) { ... }` 
- [ ] Input binding: use variable names in asm expressions
- [ ] Clobber list (optional)
- [ ] **MILESTONE**: `asm { mov rax, 42 }` generates correct instruction

### P01.07 — Codegen: Function Calls with Arguments (`P01.07`)
- [ ] Parameter passing: each arg pushed to stack or passed in registers
- [ ] SysV ABI for external calls (RDI, RSI, RDX, RCX, R8, R9, then stack)
- [ ] Return value handling (rax)
- [ ] Multiple return value tuples (return on stack)
- [ ] Recursive function calls (stack frames must nest correctly)
- [ ] **MILESTONE**: `func add(a: i64, b: i64) i64 { return a + b }` compiles correctly

### P01.08 — Codegen: For Loops and Ranges (`P01.08`)
- [ ] `for i in 0..10` → counter init, loop check, increment
- [ ] `for i in expr..expr` → dynamic range bounds
- [ ] `for item in array` → iteration over array elements
- [ ] `for ref item in array` → mutable iteration
- [ ] **MILESTONE**: `for i in 0..10 { s = s + i }` produces correct sum

### P01.09 — Codegen: Match Statements (`P01.09`)
- [ ] Integer literal pattern matching: chained cmp/je
- [ ] String pattern matching
- [ ] Range patterns: `case 0..10`
- [ ] Wildcard: `case _` → default
- [ ] Multi-condition: `case 0 | 1 | 2`
- [ ] **MILESTONE**: `match x { case 0 -> "zero" case _ -> "other" }` works

### P01.10 — Codegen: Enums with Payloads (`P01.10`)
- [ ] Enum as tagged union: discriminant + payload
- [ ] Pattern matching with payload extraction
- [ ] Enum size tracking (max variant size + discriminant)
- [ ] **MILESTONE**: `enum Option { Some(int); None }` works in match

### P01.11 — Full Expression Coverage (`P01.11`)
- [ ] Unary negation: `-x`
- [ ] Logical not: `!x`
- [ ] Bitwise ops: `&`, `|`, `^`, `~`, `<<`, `>>`
- [ ] Compound assignment: `+=`, `-=`, `*=`, `/=`
- [ ] Comparison chains: `a < b and b < c`
- [ ] Short-circuit: `&&` and `||` with early exit
- [ ] **MILESTONE**: All expression types produce correct assembly

### P01.12 — Error Handling in Codegen (`P01.12`)
- [ ] Report error for unsupported constructs with file:line:col
- [ ] Graceful recovery: emit warning, continue to next decl
- [ ] Type mismatch detection in codegen
- [ ] **MILESTONE**: Invalid constructs produce clear error messages

### P01.13 — Self-Host Test Suite Expansion (`P01.13`)
- [ ] Write `.ae` test programs for every new feature
- [ ] Each test compiles with `aether build` and verifies via ELF inspection
- [ ] Test runner: compile .ae → inspect .elf → verify expected bytes
- [ ] **MILESTONE**: 20+ .ae test programs compiling correctly

### P01.14 — Phase 1 Verification (`P01.14`)
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