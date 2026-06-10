# Aether Compiler

**A fourth-generation systems language for the Aether Operating System**

This is the Aether compiler — a from-scratch language designed to build an entire operating system without a runtime, garbage collector, or interpreter. Written in C (bootstrap phase), compiling to x86_64 freestanding ELF64 binaries via NASM, with host-native output for macOS/Linux development.

```aether
func fib(n: u64): u64 {
    if n <= 1 { return n }
    return fib(n - 1) + fib(n - 2)
}
```

### Core design principles

- **Memory management at compile time** — escape analysis, region inference, defer/scope cleanup, no GC
- **No interpreter, no runtime** — standalone freestanding binaries, runs from the first byte
- **NASM is a first-class citizen** — full inline assembly with variable binding and output constraints
- **Classes optional, automatic destruction** — zero-cost abstractions, auto-init/drop at scope exit
- **References over pointers** — `ref T`, `owned T`, `rc T`, with raw `ptr T` for low-level work
- **Host-native development** — compile and run `.ae` programs directly on macOS/Linux without a VM

```
aether build hello.ae --target host && ./hello
# or
aether run hello.ae
```

## Status

Full phase breakdown in [STATUS.md](STATUS.md). Detailed requirements in [REQUIREMENTS.md](REQUIREMENTS.md). Language specification in [SPECIFICATION.md](SPECIFICATION.md).

## Quick Start

```bash
# Build the bootstrap compiler
make

# Compile and run natively (macOS)
./build/aether --target host hello.ae -o hello
./hello

# Compile for Aether OS (freestanding ELF64)
./build/aether --target x86_64-freestanding hello.ae -o hello.elf

# One-step compile+execute
./build/aether run hello.ae

# Stop at assembly (inspect the .asm)
./build/aether hello.ae -S

# Dump AST or tokens
./build/aether hello.ae --dump-ast
./build/aether hello.ae --dump-tokens

# Run test suites
make test           # 30/30 unit tests (tokenizer + parser)
make test-host      # Native compilation + execution tests
```

## Prerequisites

- **nasm** — Netwide Assembler (for assembling generated .asm to .o)
- **x86_64-elf-ld** — ELF64 cross-linker (for freestanding output)
- **gcc** or **clang** — for building the bootstrap compiler

## Project Structure

```
compiler/
├── src/                 # C source files (bootstrap compiler)
├── include/aether/      # Header files
├── tests/               # C test suite + .ae fixture programs
├── REQUIREMENTS.md      # Language and compiler requirements
├── SPECIFICATION.md     # Full language specification with examples
├── STATUS.md            # Detailed implementation status per phase
├── PHASE_XX.md          # Phase-specific task breakdowns
└── Makefile             # Build system
```

## Credits

Aether compiler authored by **Hermes Agent (by Nous Research)** — a persistent autonomous AI agent operating under the supervision and direction of Francois Hensley.