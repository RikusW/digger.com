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
| CGA init/palette/pixel/RLE/R2 shift | reconstructed | `cga.c` |
| `incpenalty`, `levno`, `levplan`, `levof10`, `getlives`, `addlife`, `curplayer` | reconstructed | `state.c` |
| `nplayers` getter, `testpause` | getter done; pause flow noted | `state.c` |
| `sett0`, `setsoundt2`, `setsoundmode`, `sett2val`, `timer2` | reconstructed | `sound.c` |
| `startint8`, `stopint8`, `soundoff`, `initsound` | reconstructed (some probable) | `sound.c` |
| `addscore`, fixed-score wrappers, score init | structure + fragments | `game.c` |
| RNG (table + BIOS tick, shared by game/sound) | reconstructed | `rng.c` |
| `game` (whole-level loop, banner, post-level path) | reconstructed | `game.c` |
| Bonus mode (`createbonus`/`initbonusmode`/`endbonusmode`) | reconstructed | `game.c` |
| Score I/O (INT 13h sector 7) | reconstructed/mapped | `game.c` |
| `dodigger` + per-digger state + bonus block | reconstructed | `digger.c` |
| `updatefire`, `drawexplosion`, `killfire` | reconstructed | `digger.c` |
| `updatedigger` | reconstructed | `digger.c` |
| `readdir`, `getdir`, `killmonsters`, score/sound wrappers | mapped (externs) | `digger.c` |
| `movedrawspr`, `erasespr`, `drawfire`, `ggetpix`, `getfirepflag` | mapped (externs) | `digger.c` |
| `initbags`, `drawbags`, `dobags` | reconstructed | `bags.c` |
| `updatebag`, `baghitground`, `pushbag`, `pushbags`, `pushudbags`, `getgold` | reconstructed | `bags.c` |
| `removebag(s)`, `bagy`, `getbagdir`, `bagmask`, `getnmovingbags` | reconstructed | `bags.c` |
| `eatfield`, `hitemerald`, `killemerald`, `countem` | reconstructed | `field.c` |
| `diggerdie`, `checkdiggerunderbag` | reconstructed | `digger.c` |
| `domonsters`, `monai` (full update), `createmonster`, `mondie` | reconstructed | `monster.c` |
| `initmonsters`, `erasemonsters`, `nmononscr`, `incmont`, `mongold` | reconstructed | `monster.c` |
| `fieldclear`, monster collision/penalty, blob draws | reconstructed | `monster.c` |
| `pushbags`, `pushudbags`, `removebags`, `bagmask`, `eatfield`, `hitemerald` | mapped (externs) | `monster.c` |
| `initlevel`/`drawscreen`/`initchars` | reconstructed | `drawing.c` |
| `makefield`, `savefield`, `drawfield`, `makeemfield`, `drawemeralds`, `drawstatics` | reconstructed | `field.c` |
| `drawmon`, `drawmondie`, `drawdig`, `drawgold`, `drawbonus`, `drawfire` | reconstructed | `drawing.c` |
| `drawbackg`, `drawemerald`/`eraseemerald`, blob draws | reconstructed | `drawing.c` |
| `creatembspr`, `initmbspr`, `createdbfspr`, `initdbfspr` | reconstructed | `drawing.c` |
| Start-up, title screen, menu and life loop (`main` `0x82A6`) | reconstructed | `main.c` |
| Keyboard ISR, mouse sampling, `readdir`/`getdir`/`readkey`, `delay` | reconstructed | `input.c` |
| Accessors, `setdead`, `addscore`/score wrappers, `sceatm`, `killdigger`/`killmon` | reconstructed | `digger.c`/`monster.c`/`game.c` |
| Sound-event one-liners, `soundstopall`, `soundlevdone`, pause | reconstructed | `sound.c` |
| CGA wrappers (`setretr`, `gpal`, `gbackground`, `ginten`, `gclear`, `blit_title`) | reconstructed | `cga.c` |
| Title font/glyphs and `writestring` (`0x08E5`/`0x11E4`/`0x11A4`) | reconstructed | `main.c` |
| Disk-signature check (`0x026C`/`0x02F3`/`0x0321`) and hang | reconstructed | `main.c` |
| Sprite engine (`setrdrwflgs`, `collide`, `bcollides`, `putis`, `putims`, `getis`) | reconstructed | `sprite.c` |
| Pixel layer (`ggeti`, `gputi`, `gputim`, `gputich`) | reconstructed | `sprite.c` |

See `ANALYSIS.md` for the comparison and exact-match findings.
See `CGA.md` for the CGA mode/register analysis and the DOSBox failure
diagnosis.
