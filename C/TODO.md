# Decompilation TODO

## High priority

- **RNG**: identify the original random-number routine and its seed source.
  The port's multiplier `0x15A4E35` is not present in the binary; the original
  uses 16-bit multiply/divide code.  Find the callers in monster/fire/bag
  logic and reconstruct the exact sequence.
- **Game logic**: map the original functions for `digger.c`, `monster.c`,
  `bags.c`, `drawing.c`, and input/joystick to the port, then reconstruct the
  high-value routines (movement, collision, scoring).
- **Sound helpers**: decompile `sett0` (CS:0x7DD9), `setsoundmode`
  (CS:0x7EB8), `setsoundt2` (CS:0x7E7C), `sett2val` (CS:0x7FA2), and the
  timer-2 write helper (CS:0x7FD5).  They are identified by their call sites in
  `soundint`, but the bodies are not yet written.
- **Score file I/O**: locate the open/read/write path and confirm the embedded
  default-score block at `DS:0x4690` is copied or written to disk.

## Medium priority

- Decode the CGA sprite/graphics data.  The RLE block at `DS:0x0DAC` is used by
  the video routine at CS:0x17EB; identify the other bitmap/font tables in the
  data segment.
- Confirm the two-player flow: the original has `TWO`/`PLAYERS` text but no
  `SIMULTANEOUS` string; determine whether players alternate or play together.
- Map the title-screen/main-loop function at CS:0x82A6 and its callees.

## Low priority

- Name the remaining C runtime helpers (long arithmetic, formatting).
- Add a label map for every function boundary; the current `.lst` labels are
  the verified/probable subset.
- Produce a byte-for-byte diff tool that compares this reconstruction's data
  tables with `digger.com` after any future edits.
