# Phase 03 — Memory Management

**Goal:** Add automatic memory management to the Aether compiler. `defer` for scope-exit cleanup, `heap` for explicit allocation, a runtime allocator, reference types (`ref`/`owned`/`rc`), `region` blocks with arena teardown, and `T?` optional types.

**Strategy:** Each milestone adds one piece of the memory puzzle, building from simplest (scope-exit hooks) to most complex (region inference).

---

## Task Breakdown

### P03.01 — `defer` — Scope-Exit Execution (`P03.01`) 🟢
- [x] Codegen: defer stack per function (FuncDecl.defer_list)
- [x] Deferred bodies pushed at NODE_DEFER, emitted LIFO at scope exit
- [x] NODE_RETURN: save return value, emit defers, restore, epilogue
- [x] Default function return: emit defers before epilogue
- [x] Parser: defer { braced_block } and defer single_statement
- [x] DeferNode in AST (stores body), node_defer() properly assigns body
- [x] Add test fixture: `test_defer.ae` — 2 defers + return 42, LIFO verified

### P03.02 — `heap` — Explicit Heap Allocation (`P03.02`) 🟢
- [x] Parser: `heap` as unary prefix operator → `UNARY_HEAP` (NODE_UNARY_OP)
- [x] Codegen: evaluate operand, push, call `__aether_alloc(8)`, pop/store to [rax]
- [x] Codegen: `__aether_alloc` — inline mmap syscall (macOS 0x20000C5 / Linux 9)
- [x] Round size up to page boundary (4096)
- [x] `UNARY_DEREF` handler — `mov rax, [rax]`
- [x] Works on both host targets (Mach-O and ELF)
- [x] Test: `test_heap.ae` — heap 42, deref, return 42 — exit code verified

### P03.03 — Runtime Heap Allocator (`P03.03`) 🟢
- [x] Bump allocator: mmaps 64KB on first allocation, hands out from bounded arena
- [x] 16-byte alignment for all allocations
- [x] Auto-grow: mmaps another 64KB when arena exhausted (no cap)
- [x] State in .bss: `__aether_heap_start/cur/end` (3 quadwords)
- [x] `__aether_free` as no-op (bump allocator semantics — O(1) alloc, batch teardown)
- [x] Fixed NASM local label scoping (`.Lstr` → `Lstr` for cross-section references)
- [x] Test: `test_heap.ae` passes (heap 42 + deref + return 42)

### P03.04 — `ref T` / `owned T` / `rc T` — Reference Types (`P03.04`) 🟢
- [x] Parser: `ref T`, `owned T`, `rc T` type annotations in parse_type()
- [x] Uses existing `TypeNode.is_ref`/`is_owned`/`is_rc` booleans
- [x] Already works with `type_size()` (returns 8 for all pointer-sized refs)
- [x] Clean git tree: no binary artifacts tracked, output goes to /tmp/
- [x] Test: `test_ref_types.ae` — all three reference types parse and compile

### P03.05 — `region { }` — Region-Based Allocation (`P03.05`) 🟢
- [x] Parser: `region("name") { block }` → `NODE_REGION` with name + body
- [x] AST: `RegionNode` + constructor + type name
- [x] Codegen: allocates 4KB stack arena, sets `__aether_region_cur/end`
- [x] `__aether_alloc` checks region first, bumps from region arena if active
- [x] Region teardown: restore rsp (O(1), no individual frees needed)
- [x] Test: `test_region.ae` — region with heap alloc + print, verified pass

### P03.06 — Optional Types `T?` with `none` (`P03.06`) 🟢
- [x] Postfix `T?` type annotation via refactored parse_type_base() + parse_type_postfix()
- [x] Removed old prefix `?T` handler in favor of postfix `T?`
- [x] `type_size()` returns `1 + sizeof(inner_type)` for optionals
- [x] `none` literal already existed (token, AST, codegen)
- [x] Test: `test_optional.ae` — `let x: u64? = none` parses and compiles

### P03.07 — Phase 3 Verification (`P03.07`) 🟢
- [x] `make clean && make test` — 14/14 passing
- [x] `make test-host` — 10/10 host-native fixtures passing
- [x] Freestanding ELF64 output still works (hello.ae → valid ELF)
- [x] Updated STATUS.md — Phase 3 complete
- [x] Updated PHASE_03.md with final status
- [x] **MILESTONE**: Phase 3 verified and documented

---

## Technical Notes

### Heap allocation strategy
- Host target: `mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)`
  - macOS syscall: 0x20000C5
  - Linux syscall: 9
- Freestanding: simple bump allocator at a fixed address past the binary
- No fragmentation management in Phase 3 — that's an optimization concern

### `defer` implementation
- Each scope tracks a linked list of deferred statement ASTs
- On scope exit (normal, return, break, continue), emit code for each defer in LIFO order
- For function scope: defers run before the `mov rsp, rbp; pop rbp; ret` sequence

### Region implementation
- A region is just a bump arena: `{ base, current, end }`
- `heap` inside a region bumps `current`, no free needed
- Region exit: reset `current = base` (O(1))
- Compiled to inline code — no struct, just register-based bump

### Reference types at runtime
- `ref T`: just a pointer (8 bytes). The compiler ensures it doesn't outlive the borrow.
- `owned T`: a pointer + destructor call on scope exit
- `rc T`: a pointer + atomic ref count. `clone()` increments, scope exit decrements, free when 0.