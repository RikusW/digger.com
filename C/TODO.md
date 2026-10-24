# Decompilation TODO

## Completed work

- Sound hardware layer: `sett0`, `setsoundt2`, `setsoundmode`, `sett2val`,
  `timer2`, `startint8`, and `stopint8` are reconstructed in `sound.c`;
  `soundoff` is a probable identification and `initsound`'s state setup is
  verified.
- Effective `soundint` rate confirmed at ~72.8 Hz: the original stores
  `4*t0val` in `DS:0x024A`; the port stores `t0val` and subtracts `0x4000`
  after each carry.  Both give `1,193,181/16,384` ticks/s.
- Score handling: the event wrappers at `CS:0x68A2..0x68DA`
  (250/25/250/500/1000) match the port's `scorekill`, `scoreemerald`,
  `scoreoctave`, `scoregold`, and `scorebonus`; `addscore` and the wrappers
  are reconstructed in `game.c`, and the table/format/storage routines in
  `scores.c`.
- Game-state accessors in `state.c`: `incpenalty`, `levno`, `levplan`,
  `levof10`, `getlives`, `addlife`, `curplayer`, `nplayers`, and `testpause`
  are reconstructed (no fields remain probable).
- **RNG solved**: one shared generator at `CS:0x177F` uses a 256-byte
  permutation table (`DS:0x0CA7`), a counter (`DS:0x0DA7`) and the BIOS tick
  byte (`0x40:0x006C`).  Reconstructed in `rng.c`; the port's 32-bit LCG is a
  replacement and produces a different sequence.
- **Game loop reconstructed**: `game()` (`CS:0x86F1`) in `game.c`, including
  `dodigger`, `domonsters`/`monai`, `dobags`, the fireball and the level-end
  path.  `domonsters` and `monai` match the port's
  `randno(15-levof10())` / `randno(levof10()+5)` paths exactly.
- **Bonus timer solved**: `initbonusmode` (`CS:0x5528`) is
  `250 - levof10()*20`, `startbonustimeleft=20`, `bonusmode=1`; the port
  matches.  `createbonus`/`endbonusmode`/`erasebonus` are in `game.c`.
- **Score I/O solved**: there is no INT 21h file I/O.  `loadscores`
  (`CS:0x5DED`) and `savescores` (`CS:0x6521`) use INT 13h to read/write
  sector 7 of cylinder 39, head 0, into the 512-byte buffer at `DS:0x0046`.
  The format matches the port's `.sco` file.  The `DS:0x4690` block is a
  default image/legacy table with no direct code reference.

## High priority

- Nothing outstanding.  Every game, rendering, input, score, sound and CGA
  body is reconstructed: `game()`, digger/monsters/bags/fireball/field,
  the drawing and sprite engine, `main`/title/menu/font, the input layer,
  score accounting and display, high-score storage, and the CGA startup,
  block/line and pixel paths.

## CGA / DOSBox

- Resolved.  The archive item `digger_cga`'s `digger83b.zip` configuration
  runs the game correctly; its `digger.com` is byte-identical to the repo's
  apart from the high-score section.  `machine=cga` is the working machine
  type.  See `CGA.md`.
- The title bitmap format (RLE, `0xFE count value`) and the sprite/tile/font
  block format are documented in `CGA.md`.

## Medium priority

- The two-player flow is alternating: `main` initialises both players with
  three lives and `game()` runs one life at a time, switching players.
- The score call sites are mapped: `scorekill` in monster/digger/fireball
  collisions, `scoreemerald` + `sceatm` in `updatedigger`, `scoregold` in
  `getgold`, `scorebonus` on the bonus pickup, and the score wrappers in the
  monster/bag paths.

## Low priority

- Name the remaining C runtime helpers (long arithmetic at `0x8BFC`, the
  number formatter, the string routines).
- Add a label map for every function boundary; the current `.lst` labels are
  the verified/probable subset.
- Produce a byte-for-byte diff tool that compares this reconstruction's data
  tables with `digger.com` after any future edits.
