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
- **RNG solved**: one shared generator at `CS:0x177F` uses a 256-byte
  permutation table (`DS:0x0CA7`), a counter (`DS:0x0DA7`) and the BIOS tick
  byte (`0x40:0x006C`).  Reconstructed in `rng.c`; the port's 32-bit LCG is a
  replacement and produces a different sequence.
- **Game loop mapped**: `CS:0x86F1`; `dodigger` `CS:0x4939`, `domonsters`
  `CS:0x2C90`, `dobags` `CS:0x3E2D`, `monai` `CS:0x2E75`.  `domonsters` and
  `monai` match the port's `randno(15-levof10())` / `randno(levof10()+5)`
  paths exactly.
- **Bonus timer solved**: `initbonusmode` (`CS:0x5528`) is
  `250 - levof10()*20`, `startbonustimeleft=20`, `bonusmode=1`; the port
  matches.  `createbonus`/`endbonusmode`/`erasebonus` are in `game.c`.
- **Score I/O solved**: there is no INT 21h file I/O.  `loadscores`
  (`CS:0x5DED`) and `savescores` (`CS:0x6521`) use INT 13h to read/write
  sector 7 of cylinder 39, head 0, into the 512-byte buffer at `DS:0x0046`.
  The format matches the port's `.sco` file.  The `DS:0x4690` block is a
  default image/legacy table with no direct code reference.

- **Game bodies (first pass)**: `dodigger` (`CS:0x4939`) with the per-digger
  fields and bonus block in `digger.c`; `dobags` (`CS:0x3E2D`) in `bags.c`;
  `domonsters` (`CS:0x2C90`) and the `monai` (`CS:0x2E75`) direction-selection
  in `monster.c`; `initlevel`/`drawscreen`/`initchars` call order in
  `drawing.c`.

## High priority

- **All game and rendering bodies are reconstructed**: main loop, digger
  (`dodigger`/`updatedigger`/`diggerdie`), fireball, monsters, bags, field
  and emerald helpers, RNG, bonus mode, score I/O, and the drawing layer
  (`drawmon`/`drawmondie`/`drawdig`/`drawgold`/`drawfire`, `makefield`/
  `makeemfield`/`drawfield`/`drawstatics`/`drawemeralds`, `drawbackg`,
  the blob/emerald helpers, and the sprite-buffer initialisers).
- `game()` is reconstructed in `game.c`.  The sprite engine and pixel layer are in `sprite.c`; the start-up, title
  screen, menu, font and life loop are in `main.c`.  The remaining work is
  the score/menu display helpers (`0x6407`, `0x646E`, `0x8984`), the INT 10h
  teletype/scroll path, and decoding the CGA sprite/tile data and title
  bitmap.

## CGA / DOSBox

- The mode is standard CGA 320x200x4 entered by direct 3D8/3D9/CRTC writes;
  the only nonstandard timing value is the PCjr H-sync `R2=0x2C`, and the game
  rewrites R2 at runtime for a horizontal shift.  See `CGA.md`.
- Reproduce the DOSBox failure with `machine=cga` vs the default machine and
  with `cycles=fixed 400` vs 3000 before changing emulator code.
- Reconstruct the CGA block put/XOR functions at `CS:0x0628`/`CS:0x0655` and
  the worker at `CS:0x0682`.
- Reconstruct the INT 10h teletype/scroll path used for the startup message
  under the custom CRTC.
- Decode the title bitmap (`DS:0x0DAC`) and the remaining CGA sprite/font
  tables.

## Medium priority
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
