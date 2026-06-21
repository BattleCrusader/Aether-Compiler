# Phase 9 — Optimization & Polish

**Goal**: Optimize generated code quality, add developer tooling (formatter, doc generator, assembly viewer, LSP), improve error messages, and establish a benchmarking suite.

**Branch**: `feature/P09.00-optimization-and-polish`

---

## P09.01 — Constant Folding and Propagation 🟡 IN PROGRESS

Evaluate constant expressions at compile time in the AST. Replace `3 + 4` with `7`, propagate constant values through variables.

- [ ] `fold_constants()` pass: walk AST, evaluate constant sub-expressions
- [ ] Integer constant folding: `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`
- [ ] Boolean constant folding: `&&`, `||`, `!`
- [ ] Comparison constant folding: `==`, `!=`, `<`, `>`, `<=`, `>=`
- [ ] Constant propagation: track `let x = 5; x + 3` → `8`
- [ ] Conditional elimination: `if true { A } else { B }` → `A`
- [ ] Integration with existing `#run` compile-time evaluation
- [ ] Test suite: constant folding correctness

## P09.02 — Dead Code Elimination

Remove unreachable code after constant folding and propagation.

- [ ] Unreachable branch elimination (after constant condition folding)
- [ ] Dead variable elimination (assigned but never read)
- [ ] Dead function elimination (defined but never called)
- [ ] Unused import elimination
- [ ] Test suite: DCE correctness

## P09.03 — Aggressive Inlining

Inline small functions and generic monomorphizations to eliminate call overhead.

- [ ] `@force_inline` attribute support in codegen
- [ ] `@no_inline` attribute support in codegen
- [ ] Heuristic: inline functions under N instructions
- [ ] Generic monomorphization inlining (eliminate generic dispatch)
- [ ] Recursive function guard (don't infinitely inline)
- [ ] Test suite: inlining correctness

## P09.04 — Escape Analysis-Based Heap/Stack Promotion

Promote heap-allocated values to the stack when they don't escape the function.

- [ ] Escape analysis pass: track pointer/reference lifetimes
- [ ] Stack promotion: replace `heap T` with stack allocation when safe
- [ ] Integration with existing codegen (stack frame layout)
- [ ] Test suite: escape analysis correctness

## P09.05 — Region Inference → Arena Elision

Optimize region allocations by eliding unnecessary arena creation.

- [ ] Region lifetime analysis
- [ ] Small region elision (inline buffer instead of arena)
- [ ] Nested region flattening
- [ ] Test suite: region optimization correctness

## P09.06 — Devirtualization

Convert dynamic dispatch (`dyn Trait`) to static dispatch when the concrete type is known.

- [ ] Type flow analysis for `dyn Trait` variables
- [ ] Monomorphization of dynamic calls when type is known
- [ ] Fallback to vtable dispatch when type is unknown
- [ ] Test suite: devirtualization correctness

## P09.07 — Loop Unrolling and Optimization

Unroll small loops and optimize loop patterns.

- [ ] Loop unrolling for small fixed-count loops
- [ ] Loop-invariant code motion (hoist constant computations)
- [ ] Induction variable strength reduction
- [ ] Test suite: loop optimization correctness

## P09.08 — Memory Operation Fusion

Combine adjacent memory operations into wider operations.

- [ ] Adjacent load fusion (two 4-byte loads → one 8-byte load)
- [ ] Adjacent store fusion
- [ ] memset/memcpy pattern recognition
- [ ] Test suite: memory fusion correctness

## P09.09 — MIR-to-LIR Code Generation

Introduce a mid-level IR (MIR) between the AST and final NASM emission for better optimization.

- [ ] MIR definition (instruction types, basic blocks, control flow graph)
- [ ] AST → MIR translation pass
- [ ] MIR optimization passes (constant folding, DCE on MIR)
- [ ] MIR → LIR (NASM) translation
- [ ] Test suite: MIR correctness

## P09.10 — Register Allocation (Linear Scan)

Allocate virtual registers to physical registers for better code.

- [ ] Virtual register assignment in codegen
- [ ] Linear scan register allocation algorithm
- [ ] Spill/reload code generation
- [ ] Integration with existing stack frame layout
- [ ] Test suite: register allocation correctness

## P09.11 — Instruction Selection (x86_64 NASM Emission)

Select optimal x86_64 instructions for common patterns.

- [ ] Pattern matching for common idioms (x = 0 → xor x, x)
- [ ] Instruction fusion (test+cmp+jcc → single jcc)
- [ ] Addressing mode selection (lea vs add+mul)
- [ ] Test suite: instruction selection correctness

## P09.12 — `aether fmt` — Source Code Formatter

Format Aether source code with consistent style.

- [ ] Token-based reformatting (preserve structure, normalize whitespace)
- [ ] Indentation normalization (4-space)
- [ ] Brace placement (same-line for blocks)
- [ ] Spacing around operators and keywords
- [ ] Line wrapping for long lines
- [ ] `aether fmt` CLI command
- [ ] `aether fmt --check` (CI mode)
- [ ] Test suite: formatter correctness

## P09.13 — `aether doc` — Documentation Generator

Generate documentation from source comments.

- [ ] Doc comment parsing (`///` and `/** */` comments)
- [ ] HTML documentation generation
- [ ] Markdown documentation generation
- [ ] Cross-reference links (types, functions, modules)
- [ ] `aether doc` CLI command
- [ ] Test suite: doc generation correctness

## P09.14 — `aether asm` — Show Generated Assembly

Display the generated assembly for a source file.

- [ ] `aether asm <file.ae>` — compile and show NASM output
- [ ] `aether asm --target arm64 <file.ae>` — show ARM64 assembly
- [ ] `aether asm --target riscv64 <file.ae>` — show RISC-V assembly
- [ ] Source line annotations (show which source line generated which asm)
- [ ] Colorized output option
- [ ] Test suite: asm viewer correctness

## P09.15 — `aether inspect` — ELF Binary Inspection Tool

Inspect compiled ELF binaries.

- [ ] ELF header display
- [ ] Section headers display
- [ ] Symbol table display
- [ ] Disassembly of .text section
- [ ] `aether inspect <file.elf>` CLI command
- [ ] Test suite: inspector correctness

## P09.16 — LSP Server for Editor Support

Language Server Protocol implementation for IDE integration.

- [ ] LSP initialization and capabilities
- [ ] Text document synchronization
- [ ] Diagnostics (compile errors as you type)
- [ ] Completion provider
- [ ] Go-to-definition
- [ ] Hover information
- [ ] Find references
- [ ] Document symbols
- [ ] `aether lsp` CLI command
- [ ] Test suite: LSP protocol compliance

## P09.17 — Syntax Highlighting (VS Code, Vim, Helix)

Editor syntax highlighting for Aether.

- [ ] VS Code extension (TextMate grammar)
- [ ] Vim syntax file
- [ ] Helix language configuration
- [ ] Tree-sitter grammar (optional, for advanced highlighting)
- [ ] Test suite: syntax highlighting edge cases

## P09.18 — Actionable Error Messages with Suggested Fixes

Improve compiler error messages to include suggestions.

- [ ] Undefined variable: suggest similar names
- [ ] Type mismatch: suggest type conversion
- [ ] Missing semicolon: suggest insertion point
- [ ] Unused variable: suggest `_` prefix
- [ ] Unreachable code: point to the reason
- [ ] Missing return: suggest return statement
- [ ] Test suite: error message quality

## P09.19 — Performance Benchmarking Suite

Benchmark compiler performance and generated code quality.

- [ ] Compilation time benchmarks
- [ ] Generated code size benchmarks
- [ ] Runtime performance benchmarks (vs C, Rust)
- [ ] Optimization pass effectiveness metrics
- [ ] `make benchmark` target
- [ ] Historical tracking of benchmark results

---

## Legend

| Status | Meaning |
|--------|---------|
| 🟢 DONE | Completed and verified |
| 🔵 IN PROGRESS | Currently being worked on |
| 🟡 HOLD | Blocked, waiting on something else |
| 🔴 NOT STARTED | Planned but not started |
| ⚪ CANCELLED | No longer planned |
