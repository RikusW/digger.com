# C decompilation of `digger.com`

This directory holds a growing, annotated C reconstruction of the original
1983 binary. It is **not** the port in the repository root: the port is the
reference used for names and cross-checks, while every function here is
reconstructed from the 8086 disassembly.

## Conventions

- The `.COM` loads at `CS:0x100`; code addresses are CS:IP values. The
  annotated listing is `digger.lst`.
- The data segment is at `DS = CS + 0x08E7`; data offsets are DS offsets.
  The data dump is `digger.dat`.
- The original was compiled with a 16-bit small-model C compiler: `int` is
  16-bit, `long` is 32-bit, pointers are near. This workspace uses fixed-width
  types so it can be read/compiled on a modern machine.
- Names ending in `?` or marked `probable` in `SYMBOLS.md` are inferred.
  Verified routines are cross-checked byte-for-byte against the port.
- Function bodies are deliberately written close to the original control flow
  rather than optimised. Segment loads, interrupt wrappers, and the C runtime
  are omitted unless they carry game logic.

## Status

| Area | Status | File |
|---|---|---|
| Timer/speaker ISR | reconstructed | `sound.c` |
| `soundint` | reconstructed | `sound.c` |
| `music`, `musicupdate` | reconstructed | `sound.c` |
| `soundemerald`, `soundddie` | reconstructed | `sound.c` |
| `getlevch`, level data | reconstructed | `levels.c` |
| Score/bonus init | notes + fragments | `game.c` |
| `sett0`, `setsoundmode`, `setsoundt2`, `sett2val` | identified, not yet decompiled | `TODO.md` |
| Game logic (digger/monster/bags/drawing/input) | not yet started | `TODO.md` |
| RNG | not yet identified | `TODO.md` |

See `ANALYSIS.md` for the comparison and exact-match findings.
