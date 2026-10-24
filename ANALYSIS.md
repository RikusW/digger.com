# `digger.com` vs the C port

This is a reverse-engineering comparison of the original 1983 DOS binary
(`digger.com`) with the C port in this repository.

Artifacts:

- `digger.lst` — full annotated 8086 disassembly, CS:IP addresses.
- `digger.dat` — data-segment hex dump, DS offsets, known tables labelled.
- `SYMBOLS.md` — address/offset symbol tables with confidence levels.
- `CGA.md` — CGA mode/register analysis and DOSBox failure diagnosis.
- `C/` — C decompilation progress (timer/speaker ISR, sound helpers,
  music, levels, CGA, game-state accessors, scoring notes).

## How the binary is organised

- 57,600-byte DOS `.COM`; the loader puts the first file byte at `CS:0x100`.
- It is compiled 16-bit C: stack frames, near pointers, a C runtime argument
  helper at `CS:0x017B`, and the C `ctype` table at `DS:0x509A`.
- The startup sets `DS = SS = CS + 0x08E7`, so:
  - code segment: `CS:0x0100 .. CS:0x8E70` = file `0x0000 .. 0x8D70`;
  - data segment: `DS:0x0000` = file `0x8D70`;
  - `file = CS - 0x100` and `file = DS + 0x8D70`.
- The code segment also contains a few data items (the copyright string at
  `CS:0x8E40`, the saved data segment word at `CS:0x0337`).
- INT 8 is hooked at `CS:0x0339`; the handler starts at `CS:0x0397`.

## Exact data matches

| Table | Original | Port | Result |
|---|---|---|---|
| 8 levels, 150 bytes each | `DS:0x4B0E..0x4FBE` (file `0xD87E..0xDD2E`) | `leveldat[8][10][15]` | **byte-identical** |
| Background jingle | `DS:0x4866`, 291 words | `backgjingle[291]` | **byte-identical** |
| Death jingle (`dirge`) | `DS:0x4AAC`, 49 words | `dirge[]` | **byte-identical** |
| Bonus jingle `T1` | `DS:0x45E4`, 86 words | `bonusjingle[0..85]` | **identical** |
| Bonus jingle `T2` | `DS:0x4700`, 179 words | `bonusjingle[142..320]` | **identical** |
| Core UI strings | `DS:0x4FBE` onward | string literals in `main.c` etc. | **identical** |

The original stores the levels as eight separate arrays, selected by a switch
in `getlevch`; the port stores one contiguous `leveldat` array. The bytes are
the same.

### Bonus jingle and the embedded score block

The original bonus music is not contiguous:

```
DS:0x45E4  T1: 86 words (43 notes)
DS:0x4690  default high-score block: 112 bytes
DS:0x4700  T2: 179 words (89 notes, ends with 0x7D64)
```

The port's `bonusjingle` is `T1` + a **56-word bridge** + `T2`. The bridge
(port indices `86..141`) is not in the binary; it replaces the span occupied
by the original's embedded score block. `T1` and `T2` themselves match exactly.

The embedded score block decodes exactly with the port's `.sco` format:

```
's' + 10 entries of [3-byte initials][2 bytes][6-digit score]
```

The first entry is blank initials with score `1200`; the other nine are `"..."`/
`0`. The port uses the same file format (`scores.c:loadscores`), but does not
embed the 1200 default: with no `.sco` file it starts from zeros.

### Music timing

The effective `soundint` rate is 72.8 Hz. `sett0` (`CS:0x7DD9`) stores
`4*t0val` in the ISR accumulator at `DS:0x024A` (or `0xFFFF` for
`t0val >= 0x3E80`); the timer-0 IRQ rate is `1,193,181/t0val`, so the 16-bit
accumulator overflows `4*1,193,181/65,536 ≈ 72.8` times per second. The port
stores `t0val` and subtracts `0x4000` from the 16-bit accumulator after each
carry in `newsnd.c:getsample()`, which also yields
`1,193,181/16,384 ≈ 72.8 Hz`. Measured on the port: 360 calls in 5 s (72.0 Hz).

At 72.8 Hz:

| Tune | Notes | Duration |
|---|---:|---:|
| original `T1` | 43 | 5.4 s |
| original `T2` | 89 | 12.4 s |
| port's merged bonus jingle | 160 | 21.1 s |

The port's bonus mode lasts `250 - 20*level` frames; the matching original
constant has not been located yet. `T1` is short enough that, depending on the
original's frame rate, it may reach the embedded score block; the music player
would then read the score text as note/duration data. The port's bridge/`T2`
extends the music to 21.1 s, which still outlasts the port's bonus mode, so that
part is normally inaudible. Locating the original's bonus timer is an open
item.

## Exact code matches

### Timer/speaker ISR — `CS:0x0397`

The original handler:

- loads `DS/ES` from `cs:[0x337]`;
- mode `[0x248] == 0`: nothing;
- mode `1`: PWM pulse — sets speaker on, busy-loops `[0x24F]` times, speaker
  off;
- otherwise: toggles the speaker bit on port `0x61`;
- adds `[0x24A]` to `[0x24C]`; on 16-bit carry, increments `[0x24E]` and calls
  `soundint`;
- sends EOI (`0x20` to port `0x20`) and `iret`s.

This is the same model emulated by `newsnd.c` (`getsample`, `pwlut`, `lut`).
The original's PWM is a CPU busy-loop; the port approximates its duration with
timer ticks.

### `soundint` — `CS:0x7227`

The call sequence is exactly the port's:

```
musicupdate
soundemeraldupdate, soundwobbleupdate, soundddieupdate, soundbreakupdate,
soundgoldupdate, soundemupdate, soundexplodeupdate, soundfireupdate,
soundeatmupdate, soundfallupdate, sound1upupdate, soundbonusupdate
then setsoundt2 / setsoundmode+sett0, then sett2val
```

### Sound hardware helpers

The sound-control layer was decompiled and matches the port:

- `sett0` (`CS:0x7DD9`): writes the timer-0 divisor `t0val`, clamps `t0val` to
  1000 for wavetypes 1/2, sets the ISR accumulator (`4*t0val` or `0xFFFF`),
  clamps `musvol` to 1..50, stores `pulsewidth = musvol`, and calls
  `setsoundmode`.
- `setsoundt2` (`CS:0x7E7C`) and `setsoundmode` (`CS:0x7EB8`): same state
  flags (`spkrmode`, `soundt0flag`) and port `0x61` speaker-gate writes.
- `sett2val` (`CS:0x7FA2`) and `timer2` (`CS:0x7FD5`): write the timer-2
  divisor to port `0x42`.
- `startint8` (`CS:0x7EFF`) / `stopint8` (`CS:0x7F36`): hook/restore INT 8 and
  program timer 0. `soundoff` (`CS:0x7CC9`, probable) clears the speaker gate
  and sets timer 0 to `0xFFFF`; `initsound` (`CS:0x7D10`, probable) performs
  the same state initialisation as the port.

### Music — `CS:0x7A70` and `CS:0x7B02`

`music(tune)` sets the same state and the same three envelope parameter sets as
the port. `musicupdate` has the same attack/decay/release state machine and the
same duration multipliers (`*3`, `*6`, `*10`) and `0x7D64` sentinel. The three
table bases are `DS:0x45E4`, `DS:0x4866`, `DS:0x4AAC` — bonus `T1`, background
jingle, dirge.

### Individual sounds

- `soundemerald` (`CS:0x76F7`): same eight frequencies
  (`0x8E8, 0x7F0, 0x712, 0x6AC, 0x5F2, 0x54C, 0x4B8, 0x474`), duration 7,
  state 0, flag 1.
- `soundddie` (`CS:0x7933`): state 0, value 20000, flag 1.
- Score event wrappers at `CS:0x68A2/0x68B0/0x68BE/0x68CC/0x68DA` add
  250/25/250/500/1000. These match the port's `scorekill`, `scoreemerald`,
  `scoreoctave`, `scoregold`, and `scorebonus`. The port's simultaneous
  `scorekill2` (125+125) is an addition.
- The extra-life step uses `0x4E20` (20000), matching the port's
  `bonusscore`.

### `getlevch` — `CS:0x8001`

The original switches on levels 1..8 and returns
`levelData[level-1][y*15 + x]`, with each level in its own 150-byte array.
The port's `leveldat` flattening is equivalent.

### Game-state utilities — `CS:0x8A32..CS:0x8B55`

The high-call-count region is the core state layer and matches the port:

- `incpenalty` (`CS:0x8A32`, 52 callers) increments `DS:0x5014`; this is the
  port's `incpenalty()`.
- `levno` (`CS:0x8ABD`) returns the level at `DS:0x4FE2 + 8*curplayer`.
- `levplan` (`CS:0x8A65`) calls `levno` and applies `(l&3)+5` for `l>8`.
- `levof10` (`CS:0x8A8D`) returns `min(level,10)`.
- `getlives` (`CS:0x8AF0`) and `addlife` (`CS:0x8B0C`) read/increment the two
  lives fields at `DS:0x4FE0`/`DS:0x4FE8`; `addlife` calls `drawlives`.
- The `curplayer` getter (`CS:0x8B45`), probable `nplayers` getter
  (`CS:0x8B4D`), and `testpause` (`CS:0x8B55`) match the port's state access
  and pause flow.

### CGA video — `CS:0x03F4..0x05EB`, `CS:0x1128/0x1155`

The original uses **standard CGA mode 4** (320×200, 4 colours, 2 bpp,
interleaved) but enters it by direct register writes, not INT 10h:

```
3D8h = 02h
CRTC R0..R13 = 38 28 2C 0A 7F 06 64 70 02 01 20 00 00 00
3D9h = 20h
fill 16KB framebuffer with colour 0
3D8h = 0Ah
```

The only nonstandard timing value is the **PCjr H-sync `R2=0x2C`** (IBM CGA
mode 4 uses `0x2D`); the game also rewrites `R2` at runtime (0x2A..0x2E) for
a horizontal shift. The framebuffer address formula is
`(y&~1)*40 + (y&1)*0x2000 + (x>>2)`, i.e. the standard CGA even/odd bank
layout. `R9=1` is normal for mode 4. See `CGA.md` for the DOSBox failure
analysis and fix directions.

## Differences

### Port additions

- VGA/EGA graphics (`vgagrafx.c`, `alpha.c`), SDL video/keyboard/timer,
  SoundBlaster/DMA (`newsnd.c`), recordings, INI/command-line options, CGA and
  BIOS-palette paths.
- Gauntlet mode, simultaneous two-player mode, and custom level files.
- The port's `getlevch` adds a simultaneous-mode special case
  (`diggers == 2`, levels 3/4, `(6,9)`/`(8,9)` → `'H'`) and a `levfflag` guard;
  neither is in the original's `getlevch`.
- Port-only text: command-line help, INI keys, `SIMULTANEOUS`, `GAUNTLET`,
  `TWO PLAYER`, `TIME UP`, key names, version/copyright strings.
- The original has `TWO`/`PLAYERS` text, so a two-player option exists, but
  there is no `SIMULTANEOUS` or `GAUNTLET` string; the port's simultaneous mode
  is an extension.

### Behavioral differences

- **RNG**: the port's `randno`/`randnos` use a 32-bit LCG with multiplier
  `0x15A4E35`. That constant appears nowhere in the binary. There is no
  `idiv word [bp+disp]`, and the 32-bit divide helper is only called from
  number formatting, so the original is likely a 16-bit or inlined generator,
  or a table-driven one. Its exact algorithm has not been identified yet.
  The port also seeds from `getlrt()`, which returns 0 on SDL, so its random
  sequence is deterministic.
- **PWM emulation**: the original's mode-1 pulse is a CPU busy-loop whose
  length depends on CPU speed (hence `calibrate()`/`getkips()`); the port
  approximates it with `pwlut` and the sample-level `getsample` model, plus the
  speaker filter/oversampling added for SDL.
- **Default scores**: the original embeds the 1200-score block; the port reads
  a `.sco` file and otherwise starts at zero.
- **CGA startup text**: the messages are at `DS:0x0263` / `DS:0x0322`
  (`file 0x8FD3` / `0x9092`; the printable text starts at file `0x9005` /
  `0x90C4`). They are printed via INT 10h after the custom CRTC is installed;
  the SDL port does not have them.

## Open items / next work

- Identify the original RNG routine and its seed source.
- Locate the original bonus-mode timer (the port uses `250 - 20*level` frames).
- Map the main game-logic functions (`digger`, `monster`, `bags`, `drawing`,
  input/joystick) to the port.
- Decode the CGA sprite/graphics data and its RLE title bitmap.
- Reproduce the DOSBox CGA failure with `machine=cga` vs the default machine
  and `cycles=fixed 400` vs 3000; see `CGA.md`.
- Reconstruct the CGA block put/XOR functions and the INT 10h teletype/scroll
  path.
- Confirm the original two-player flow (alternating vs simultaneous).
- Map the score file I/O and the remaining score display functions.

## Method

`objdump -b binary -m i8086 -D -M intel --adjust-vma=0x100
--stop-address=0x8e70 digger.com` produced the code listing; data tables
were located by searching for the port's arrays and by following the
disassembly's DS-relative references. Capstone 5.0.7 was used for targeted
disassembly. Cross-checks used the matching C port as a naming reference, not
as a source of truth: every claim above is backed by bytes in the binary.
