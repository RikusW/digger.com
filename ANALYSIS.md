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

### Random numbers — `CS:0x177F`

The original has one shared generator for the game and the sound engine:

```
seed = ++DS:0x0DA7;
tick = byte at 0x40:0x006C;            (BIOS timer low byte)
r    = DS:0x0CA7[(seed + tick) & 0xFF];  (256-byte permutation table)
return (r * (n + 1)) >> 8;
```

Call sites: `domonsters` (`randno(15-levof10())`), `monai`
(`randno(levof10()+5)`), and `soundfireupdate` (the t2 jitter). The port's
`randno`/`randnos` 32-bit LCG (`0x15A4E35`) and separate seeds are a
replacement; its sequence differs. See `C/rng.c`.

### Main game loop — `CS:0x86F1`

`game()` is entered **once per life/level** and runs an internal frame loop;
it does not return each frame:

- copy-protection re-check (`DS:0x5016` must be `0x45`, else hang);
- if `levnotdrawn` (`DS:0x5010`) is set: `drawscreen`; if the banner flag
  (`DS:0x5012`) is set, build `"PLAYER n"` at `DS:0x4FEE`, clear the top
  line, flash it in colours 1/2/3 fifteen times, then `addscore(0)`;
  otherwise `initchars` (a re-entry after a death on the same field);
- frame loop: `readdir`, `dodigger`, `domonsters`, `dobags`, penalty delay
  `(8-penalty)*5` or `incmont(penalty-8)`, `testpause`, `checklevdone`;
- on death/level end: `erasediggers`, `musicoff`, a 20-frame bag-settle loop
  (`getnmovingbags`, `dobags`/`dodigger`/`domonsters`), then `playmusic(0)`,
  `killfire`, `erasebonus`, `cleanupbags`, `savefield`, `erasemonsters`,
  `soundlevdone`, level advance on `countem()==0` or `levdone`, life loss
  and `gameover` (`0x610B`) when the last life goes.

`domonsters` loops six monsters (stride `0x28`, table `DS:0x3B1C`), applies
`hnt > 10-levof10()`, and calls `monai` (`CS:0x2E75`), which contains the
port's `randno(levof10()+5)==1` direction swap. `initlevel` (`CS:0x89A4`),
`drawscreen` (`CS:0x89CD`), `initchars` (`CS:0x89E4`), and `initscores`
(`CS:0x5DE0`) map to the port. The post-level path matches the port's
`getnmovingbags` loop plus the soundstop/erasebonus/cleanupbags/savefield/
erasemonsters/countem sequence. `game()` is reconstructed in `C/game.c`.

### Game-logic bodies

- **`dodigger` `CS:0x4939`** — uses single-digger globals: x `0x3FDE`,
  y `0x3FE0`, h `0x3FE2`, v `0x3FE4`, rx `0x3FE6`, ry `0x3FE8`, mdir `0x3FEA`,
  ddir `0x3FEC`, dir `0x3FEE`, bagtime `0x3FF0`, alive `0x3FF2`, digvisible
  `0x3FF4`, notfiring `0x3FF6`, rechargetime `0x3FF8`, fx `0x3FFA`,
  fy `0x3FFC`, fdir `0x3FFE`, expsn `0x4000`, deathstage `0x4002`,
  deathbag `0x4004`, emocttime `0x4016`. It calls `drawdig` `0x263F`,
  `updatefire` `0x4A3E`, `drawexplosion` `0x4E66`, `updatedigger` `0x4F07`,
  `diggerdie` `0x5218`, and contains the bonus block
  (`bonusmode`/`bonustimeleft`/`startbonustimeleft`).
  The port's `digdat[]` loop and simultaneous two-player mode are extensions.
  Reconstructed in `C/digger.c`.
- **`updatedigger` `CS:0x4F07`** — `readdir`/`getdir`, input gating by `rx`/`ry`,
  the edge stop, `eatfield`, the blob move, `hitemerald` (score/`soundem`/
  `soundemerald`, cooldown `emocttime=9`), `drawdig`, the bag push (with
  `bagtime++` and the rollback on failure), bonus-mode monster kills
  (`killmonsters` + `soundeatm`/`sceatm` loop), the bonus pickup
  (`scorebonus` + `initbonusmode`), and the coordinate update. The original
  leaves the local push flag uninitialised on the vertical-bag path; the port
  initialises it to TRUE. Reconstructed in `C/digger.c`.
- **Collision mask** — bit 0 digger, bits 1..7 bags, bits 8..13 monsters,
  bit 14 bonus, bit 15 fireball. This differs from the port's
  `clfirst[]`/`clcoll[]` type ordering.
- **`soundemerald` `CS:0x76F7`** — the argument is only a start flag, not an
  index: `0` resets the scale to `0x8E8`; nonzero steps `DS:0x45A2` through
  `0474/04B8/054C/05F2/06AC/0712/07F0/08E8` and calls `scoreoctave` on wrap.
  The port's `emn` counter plus `emocttime` reproduces this, but moves the
  octave score into `updatedigger`.
- **`updatefire` `CS:0x4A3E`** — fire launch (`levof10()*3+60` recharge,
  `fx`/`fy` from `dir`, sprite 15), the 7-pixel pixel probes (`ggetpix`
  `0x56A6`), `drawfire` `0x24C6`, monster hits (`killmon`/`scorekill`),
  the `0x40FE` impact mask, and the edge/impact explosion tests. `drawexplosion`
  (`0x4E66`) and `killfire` (`0x4EE3`) match the port. Note `updatefire` uses
  `dir` `0x3FEE`, and `firepressed` `0x0663` is separate from the
  `getfirepflag` byte at `DS:0x40BC`.
- **`dobags` `CS:0x3E2D`** — bag records stride `0x28`: x `0x3E9A`, y `0x3E9C`,
  h `0x3E9E`, v `0x3EA0`, dir `0x3EA6`, wobbling `0x3EA8`, exist `0x3EAE`,
  gt `0x3EB0`, goldtime `0x3FDC`. Calls `soundbreak`, `drawgold`, `incpenalty`,
  `removebag`, `getfield`, `updatebag`; matches the port's `dobags`.
  Reconstructed in `C/bags.c`.
- **`monai` `CS:0x2E75`** — the whole per-monster update matches the port:
  hobbin conversion, digger-0 chase target (there is no per-monster `chase`
  field in the original), priority list, `bonusmode` reversal, `reversedir`,
  `randno(levof10()+5)` swap, `fieldclear` priority checks, the screen-edge
  stop, `eatfield`/blob drawing and the +4/-4/+3/-3 movement, `hitemerald`,
  `stime`/`hnt`, `drawmon`, the monster/bag/digger collision blocks, and the
  final `h`/`v`/`xr`/`yr` update. Fields: flag `0x3B1C`, x `0x3B1E`, y
  `0x3B20`, h `0x3B22`, v `0x3B24`, xr `0x3B26`, yr `0x3B28`, dir `0x3B2A`,
  hdir `0x3B2C`, t `0x3B2E`, alive `0x3B30`, nob `0x3B32`, hnt `0x3B34`,
  death `0x3B36`, bag `0x3B38`, dtime `0x3B3A`, stime `0x3B3C`.
  `domonsters` (`CS:0x2C90`) also contains the spawn timer
  (`nextmontime`, `createmonster`) and the `unbonusflag`/`createbonus`
  step, matching the port. See `C/monster.c`.
- **Monster helpers** — `initmonsters` `0x2BC1`, `erasemonsters` `0x2C58`,
  `mondie` `0x3682`, `nmononscr` `0x3ABB`, `monleft` `0x3AA5`, `incmont`
  `0x3AEF`, `mongold` `0x3B26`, `fieldclear` `0x3815`, collision routines
  `0x3795`/`0x3767`, blob draws `0x21CC`/`0x2214`/`0x225C`/`0x22A2`,
  `killmon` `0x39A9`, `killmonsters` `0x3A40`, `squashmonster(s)`
  `0x3A7C`/`0x39E7`, `checkmonscared` `0x3962`, `killdigger` `0x5668`,
  `bagmask` `0x4774`, `removebags` `0x47B5`.
- **`diggerdie` `CS:0x5218`** — states 1..5: settle on the bag, frames
  14..10 with `music(2)`, the 3/5/6/6/5/3/0 fall arc, then state 4 only
  decrements `deathtime` and calls `setdead(1)`. Unlike the port, the
  original does **not** deduct lives or respawn here: the main loop does
  that (`0x8936`: `dec lives`, `drawlives`, game over when it reaches 0),
  and the per-player dead flag lives at `0x4FDE + 8*curplayer`.
- **Bag physics** — `updatebag` `0x3FED` (wobble/fall), `baghitground`
  `0x4296`, `pushbag` `0x433B`, `pushbags` `0x466C`, `pushudbags` `0x46B2`,
  `removebag(s)` `0x4703`/`0x47B5`, `getgold` `0x4853`, `bagy` `0x4730`,
  `getbagdir` `0x4749`, `bagmask` `0x4774`, `getnmovingbags` `0x47F8`.
  Records are 0x28 bytes with `x,y,h,v,xr,yr,dir,wobbling,wt,unfallen,exist,
  gt,fallh`; `initbags` `0x3B31` scans `getlevch(x,y,levplan())=='B'` and
  saves two copies at `0x3C1A`/`0x3D5A` for the players.
- **Field and emeralds** — the field is 15x10 16-bit cells at `DS:0x4438`
  (row stride `0x1E`): bits 0..4 horizontal sub-cells, 6..11 vertical,
  `0x2000` solid. `eatfield` `0x6DFE` clears the covered sub-cells and
  `0x2000` when the cell empties. Emeralds live in a 15x10 byte map at
  `DS:0x4018` with mask `DS:0x40AE`; `hitemerald` `0x57F1` uses
  `embox[8]={8,12,12,9,16,12,6,9}`, `killemerald` `0x5AE0` clears the cell
  below, and `countem` `0x5A94` counts the remaining ones.
- **Drawing calls** — `initlevel` `CS:0x89A4` calls `makefield` `0x6A48`,
  `makeemfield` `0x56E8`, `initbags` `0x3B31`; `drawscreen` `CS:0x89CD` calls
  `creatembspr` `0x1826`, `drawstatics` `0x6B31`, `drawbags` `0x3C93`,
  `drawemeralds` `0x5777`, `initdigger` `0x48A0`, `initmonsters` `0x2BC1`;
  `initchars` `CS:0x89E4` calls `initmbspr` `0x1A03`, `initdigger`,
  `initmonsters`. See `C/drawing.c`.

- **Start-up and title screen** — `main` `CS:0x82A6`: video/sound init,
  a 10-try disk signature check (`diskcheck` `0x026C`: sector-2 byte `0x0E`
  plus BIOS `0x40:0x42..0x44` = `40 20 20`; failure hangs in `0x08BE0`),
  `loadscores`, `initsound`, then the title. The title blits the RLE bitmap
  (`0x17EB`), prints `"D I G G E R"`, and runs a scripted animation:
  nobbin/hobbin/digger runs at frames `0x32/0x5A/0x82`, gold `0xB2`,
  emerald `0xC6`, bonus `0xDA`, with labels `NOBBIN`/`HOBBIN`/`DIGGER`/
  `GOLD`/`EMERALD`/`BONUS`. Frame 250 wraps. After a key (`readkey`
  `0x5C5E`), the life loop starts each player with level 1 and 3 lives and
  calls `initmbspr` + `game` per frame. The title text uses a 64-entry
  font (`CS:0x1523`) of `[w][h][2bpp]` glyphs; `writestring` advances 12
  pixels per character (`drawglyph` `0x08E5`, `drawchar` `0x11E4`).
- **Sprite engine** — 16 sprites plus a misc slot; per-sprite property arrays
  at `DS:0x03CF..0x0603` (`sprrdrwf`, `sprrecf`, `sprenf`, sources, boxes,
  sizes) and the draw-order table at `0x0627` (0..15). `setrdrwflgs`
  (`0x0A90`) recursively marks overlapping sprites and retries the test with
  a +320/-2 and -640/+4 edge shift; `collide` (`0x0B05`) and `bcollide`
  (`0x0B6E`) are the overlap tests; `bcollides` (`0x0D48`) fills the
  `first[5]`/`coll[16]` lists. The pixel layer is `ggeti` `0x0CD2` (screen
  → saved buffer), `gputi` `0x0BFF` (restore), `gputim` `0x0CF3`
  (`(screen & mask) | data`) and `gputich` `0x0C1E`, using the standard CGA
  interleaved address `(y&~1)*40 + (y&1)*0x2000 + x/4`. Reconstructed in
  `C/sprite.c`.
- **Rendering layer** — `creatembspr` `0x1826` / `initmbspr` `0x1A03`
  build the per-sprite frame buffers (sprites 1..7 = bags, 8..13 = monsters)
  and call `createdbfspr` `0x20F6` / `initdbfspr` `0x2167` for the digger,
  bonus and fireball. `drawmon` `0x1B84` and `drawdig` `0x263F` animate legs
  (per-monster `0x16CC/0x16D8`, digger `0x2700/0x2702`, fireball `0x2704`)
  and select 4x15 frames from CS source pairs; `drawmondie` `0x1DDB` picks
  the 4x15/4x14 death frames; `drawgold` `0x1E9D` has seven 4x15 frames;
  `drawfire` `0x24C6` uses 2x7 frames. The field walls come from
  `drawfield` `0x6C3E` (matches the port), the tiled background from
  `drawbackg` `0x237A` (eight 5x4 tile sets chosen by `levplan()`), and the
  emerald/erase/blob helpers use 4x10, 6x6, 6x8, 2x18 misc sprites. Low-level
  primitives: `setspr` `0x0804`, `setspr2` `0x0754`, `drawspr` `0x07E4`,
  `initmiscspr` `0x084F`, `drawmiscspr` `0x08AD`, `getis` `0x0876`.

### Bonus mode — `CS:0x5510..0x5575`

`initbonusmode` (`CS:0x5528`) sets `bonusmode=1`, `erasebonus`, `ginten(1)`,
`bonustimeleft = 250 - levof10()*20` (`DS:0x400E`),
`startbonustimeleft=20` (`DS:0x400C`), and `DS:0x4014=1`. This is exactly the
port's formula. `createbonus` (`CS:0x5510`), `endbonusmode` (`CS:0x5562`), and
`erasebonus` (`CS:0x5575`) match the port's structure.

### Score storage — INT 13h sector 7

There is **no INT 21h** in the binary. `loadscores` (`CS:0x5DED`, file
`0x5CED`) calls `CS:0x028F`:

```
INT 13h AH=2, drive 0, head 0, cylinder 0x27 (39), sector 7, 1 sector
ES:BX = DS:0x0046
```

and parses `'s'` + ten entries of `[3 initials][2 bytes][6 score digits]`
into `DS:0x4134` (initials) and `DS:0x4108` (32-bit scores). `savescores`
(`CS:0x6521`) builds the same 512-byte buffer and uses `CS:0x02B8` (INT 13h
AH=4 verify, then AH=3 write). The port uses a `.sco` file with the same
format.

The 112-byte block at `DS:0x4690` (first entry score 1200) has no direct code
reference; it is a default/legacy score image. The sector-2 read/check
(`CS:0x0321` / `CS:0x02F3`) is the disk signature / copy-protection check.

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

- **RNG**: the original uses the shared table+BIOS-tick generator at
  `CS:0x177F` (256-byte permutation at `DS:0x0CA7`, counter at `DS:0x0DA7`,
  tick byte at `0x40:0x006C`). The port's `randno`/`randnos` use a 32-bit LCG
  with multiplier `0x15A4E35` and separate seeds; that constant is not in the
  binary, so the port's random sequence differs. On SDL `getlrt()` returns 0,
  making the port's sequence deterministic.
- **PWM emulation**: the original's mode-1 pulse is a CPU busy-loop whose
  length depends on CPU speed (hence `calibrate()`/`getkips()`); the port
  approximates it with `pwlut` and the sample-level `getsample` model, plus the
  speaker filter/oversampling added for SDL.
- **Score storage**: the original saves high scores in raw disk sector 7
  (INT 13h); the port uses a `.sco` file with the same 512-byte format and
  starts at zeros if the file is absent. The embedded 1200-score block at
  `DS:0x4690` is a default/legacy image with no direct code reference.
- **CGA startup text**: the messages are at `DS:0x0263` / `DS:0x0322`
  (`file 0x8FD3` / `0x9092`; the printable text starts at file `0x9005` /
  `0x90C4`). They are printed via INT 10h after the custom CRTC is installed;
  the SDL port does not have them.

## Open items / next work

- Decode the CGA sprite/tile data behind the source addresses
  (`CS:0x16E4..0x3B00`) and the title bitmap.
- Reconstruct the remaining score/menu display helpers (`0x6407`, `0x646E`,
  `0x8984`) and the INT 10h teletype/scroll path used under the custom CRTC.
- Decode the CGA sprite/graphics data and its RLE title bitmap.
- Reproduce the DOSBox CGA failure with `machine=cga` vs the default machine
  and `cycles=fixed 400` vs 3000; see `CGA.md`.
- Reconstruct the CGA block put/XOR functions and the INT 10h teletype/scroll
  path.
- Confirm the original two-player flow (alternating vs simultaneous).
- Map the remaining score display functions and the default-sector
  initialization path.

## Method

`objdump -b binary -m i8086 -D -M intel --adjust-vma=0x100
--stop-address=0x8e70 digger.com` produced the code listing; data tables
were located by searching for the port's arrays and by following the
disassembly's DS-relative references. Capstone 5.0.7 was used for targeted
disassembly. Cross-checks used the matching C port as a naming reference, not
as a source of truth: every claim above is backed by bytes in the binary.
