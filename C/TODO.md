# Decompilation TODO

## Completed in this pass

- Sound hardware layer: `sett0`, `setsoundt2`, `setsoundmode`, `sett2val`,
  `timer2`, `startint8`, `stopint8`, `soundoff` (probable), and `initsound`
  (probable) are reconstructed in `sound.c`.
- Effective `soundint` rate confirmed at ~72.8 Hz: the original stores
  `4*t0val` in `DS:0x024A`; the port stores `t0val` and subtracts `0x4000`
  after each carry.  Both give `1,193,181/16,384` ticks/s.
- Score event wrappers at `CS:0x68A2..0x68DA` (250/25/250/500/1000) match the
  port's `scorekill`, `scoreemerald`, `scoreoctave`, `scoregold`, and
  `scorebonus`; `addscore` structure matches `scores.c`.
- Game-state accessors in `state.c`: `incpenalty`, `levno`, `levplan`,
  `levof10`, `getlives`, `addlife`, and `curplayer` match the port.  The
  `nplayers` getter and `testpause` flow are identified (some fields probable).

## High priority

- **RNG**: identify the original random-number routine and its seed source.
  The port's multiplier `0x15A4E35` is not present in the binary, there is no
  `idiv word [bp+disp]`, and the long-divide helper is only called from number
  formatting.  The original is likely a 16-bit or inlined generator, or a
  table.  Continue by tracing calls from the monster/fire update functions.
- **Game logic**: map the original functions for `digger.c`, `monster.c`,
  `bags.c`, `drawing.c`, and input/joystick to the port, then reconstruct the
  high-value routines (movement, collision, scoring).
- **Bonus-mode timer**: locate the original's bonus duration.  The port uses
  `250 - 20*level` frames; this decides whether the 5.4 s `T1` bonus jingle
  reaches the embedded score block at `DS:0x4690`.
- **Score file I/O**: locate the open/read/write path and confirm the embedded
  default-score block at `DS:0x4690` is copied or written to disk.

## Medium priority

- Decode the CGA sprite/graphics data.  The RLE block at `DS:0x0DAC` is used by
  the video routine at CS:0x17EB; identify the other bitmap/font tables in the
  data segment.
- Confirm the two-player flow: the original has `TWO`/`PLAYERS` text but no
  `SIMULTANEOUS` string; determine whether players alternate or play together.
- Map the title-screen/main-loop function at CS:0x82A6 and its callees.
- Map the score event call sites to the actual game events (which wrapper is
  called on which collision/bonus).

## Low priority

- Name the remaining C runtime helpers (long arithmetic, formatting).
- Add a label map for every function boundary; the current `.lst` labels are
  the verified/probable subset.
- Produce a byte-for-byte diff tool that compares this reconstruction's data
  tables with `digger.com` after any future edits.
