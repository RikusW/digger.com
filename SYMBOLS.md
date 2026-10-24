# Symbols in `digger.com`

Address conventions:

- The `.COM` is loaded at `CS:0x100`, so a file offset `F` has code address `CS = F + 0x100`.
- The startup sets `DS = SS = CS + 0x08E7`, so data address `DS:D` is file offset `0x8D70 + D`.
- `digger.lst` uses CS addresses; `digger.dat` uses DS offsets.

Confidence: **verified** = cross-checked against the matching C port and the disassembly; **mapped** = identified from the `soundint` call order; **probable** = inferred from structure/constants.

## Code

| CS | file | Name | Confidence | Notes |
|---|---:|---|---|---|
| `0x0337` | `0x0237` | saved data segment | verified | Word in the code segment; `cs:[0x337]` is loaded by the ISR. |
| `0x0339` | `0x0239` | INT 8 hook setup | verified | Saves the old vector at `DS:0x0251/0x0253` and points vector `0:0x20` at `CS:0x0397`. |
| `0x0397` | `0x0297` | `timer_isr` | verified | Toggles/speaker-pulses port `0x61`; `timercount += timerrate`; on 16-bit overflow calls `soundint`. |
| `0x03F4` | `0x02F4` | CGA video init entry | verified | Calls `CS:0x03FC`. |
| `0x03FC` | `0x02FC` | CGA video init | verified | Probes CGA, calls `cga_init`, prints the startup message. |
| `0x0418` | `0x0318` | `cga_probe` | verified | Writes/reads `B800:0000` with `0x55`. |
| `0x0429` | `0x0329` | `cga_init` | verified | `3D8=2`, CRTC table, `3D9=0x20`, clear, `3D8=0x0A`. |
| `0x0456` | `0x0356` | `cga_print_text` | verified | INT 10h cursor/teletype; checks BIOS equipment word `0x30`. |
| `0x048A` | `0x038A` | `cga_mode_off` | verified | Wait retrace; `3D8=2`. |
| `0x04A0` | `0x03A0` | `cga_mode_on` | verified | Wait retrace; `3D8=0x0A`. |
| `0x04AE` | `0x03AE` | `cga_clear` | verified | Calls the fill worker. |
| `0x04B9` | `0x03B9` | `cga_clear_fill` | verified | Replicates 2-bit colour, fills 16KB with video off/on. |
| `0x04E7` | `0x03E7` | `cga_palette` | verified | `3D9` bit 5. |
| `0x050B` | `0x040B` | `cga_wait_retrace` | verified | Polls `3DA` bit 3 when `DS:0x03B3` is set. |
| `0x0520` | `0x0420` | `cga_set_background` | verified | `3D9` bits 0-3. |
| `0x054B` | `0x044B` | `cga_set_intensity` | verified | `3D9` bit 4. |
| `0x0556` | `0x0456` | CGA intensity worker | verified | Called by `cga_set_intensity`. |
| `0x057A` | `0x047A` | `cga_putpixel` | verified | Replace mode; bounds 320x200. |
| `0x0595` | `0x0495` | `cga_xor_pixel` | verified | XOR mode. |
| `0x05B0` | `0x04B0` | CGA pixel worker | verified | Bounds/address/apply. |
| `0x05C3` | `0x04C3` | CGA pixel bounds | verified | `x < 0x140`, `y < 0xC8`. |
| `0x05CF` | `0x04CF` | CGA pixel address | verified | `(y&~1)*40 + (y&1)*0x2000 + x/4`. |
| `0x05EB` | `0x04EB` | CGA pixel apply | verified | 2bpp mask/write, leftmost in bits 7-6. |
| `0x0628` | `0x0528` | CGA block put | probable | Five args; calls worker `CS:0x0682`. |
| `0x0655` | `0x0555` | CGA block XOR | probable | Five args; calls worker `CS:0x0682`. |
| `0x0682` | `0x0582` | CGA block worker | probable | Computes width/height from two corners. |
| `0x1128` | `0x1028` | `cga_shift_inc` | verified | CRTC R2++, clamp `0x2A..0x2E`. |
| `0x1155` | `0x1055` | `cga_shift_dec` | verified | CRTC R2--, clamp `0x2A..0x2E`. |
| `0x17EB` | `0x16EB` | `cga_blit_rle` | verified | `DS:0x0DAC` to `B800`, 0x4000 bytes, `0xFE` escape. |
| `0x5EEA` | `0x5DEA` | `score_init`? | probable | Initializes 32-bit score fields to 0 and next-bonus fields to `0x4E20` (20000). |
| `0x5FA1` | `0x5EA1` | `score_add`? | probable | Adds to score; the `0x4E20` step at `CS:0x6034` is the extra-life bonus. |
| `0x68A2` | `0x67A2` | scorekill/scoreoctave wrapper | probable | Calls `score_add(250)`. |
| `0x68B0` | `0x67B0` | scoreemerald wrapper | probable | Calls `score_add(25)`. |
| `0x68BE` | `0x67BE` | scoreoctave/scorekill wrapper | probable | Calls `score_add(250)`. |
| `0x68CC` | `0x67CC` | scoregold wrapper | probable | Calls `score_add(500)`. |
| `0x68DA` | `0x67DA` | scorebonus wrapper | probable | Calls `score_add(1000)`. |
| `0x7CB3` | `0x7BB3` | `soundpause` | verified | Sets `DS:0x45D2=1`. |
| `0x7CBE` | `0x7BBE` | `soundpauseoff` | verified | Sets `DS:0x45D2=0`. |
| `0x8A32` | `0x8932` | `incpenalty` | verified | `DS:0x5014++`; 52 callers. |
| `0x8A65` | `0x8965` | `levplan` | verified | Calls `levno`; `(l&3)+5` for `l>8`. |
| `0x8A8D` | `0x898D` | `levof10` | verified | `min(level,10)`. |
| `0x8ABD` | `0x89BD` | `levno` | verified | Level of `curplayer` (`DS:0x4FE2+8*curplayer`). |
| `0x8AF0` | `0x89F0` | `getlives` | verified | Reads the two lives fields. |
| `0x8B0C` | `0x8A0C` | `addlife` | verified | Increments lives, calls `drawlives`. |
| `0x8B45` | `0x8A45` | `curplayer` getter | verified | Returns `DS:0x500C`. |
| `0x8B4D` | `0x8A4D` | `nplayers` getter | probable | Returns `DS:0x500E`. |
| `0x8B55` | `0x8A55` | `testpause` | probable | Pause key handling; see `state.c`. |
| `0x7227` | `0x7127` | `soundint` | verified | Same call order as the port's `soundint()`. |
| `0x74C7` | `0x73C7` | `soundbreakupdate` | mapped | 4th call in `soundint`. |
| `0x74EC` | `0x73EC` | `soundwobbleupdate` | mapped | 2nd call in `soundint`. |
| `0x758F` | `0x748F` | `soundfireupdate` | mapped | 8th call in `soundint`. |
| `0x760B` | `0x750B` | `soundexplodeupdate` | mapped | 7th call in `soundint`. |
| `0x765E` | `0x755E` | `soundbonusupdate` | mapped | 13th call in `soundint`. |
| `0x76D7` | `0x75D7` | `soundemupdate` | mapped | 6th call in `soundint`. |
| `0x76F7` | `0x75F7` | `soundemerald` | verified | Sets the same 8 frequencies and `duration=7`, `n=0`, `flag=1`. |
| `0x779B` | `0x769B` | `soundemeraldupdate` | mapped | 1st call in `soundint`. |
| `0x782A` | `0x772A` | `soundgoldupdate` | mapped | 5th call in `soundint`. |
| `0x7933` | `0x7833` | `soundddie` | verified | Sets `n=0`, `value=20000`, `flag=1`. |
| `0x794A` | `0x784A` | `soundddieupdate` | mapped | 3rd call in `soundint`. |
| `0x7A1C` | `0x791C` | `sound1upupdate` | mapped | 12th call in `soundint`. |
| `0x7A70` | `0x7970` | `music` | verified | Same three envelope parameter sets as the port. |
| `0x7B02` | `0x7A02` | `musicupdate` | verified | Same state machine, `*3/*6/*10` duration scaling, sentinel `0x7D64`. |
| `0x7CC9` | `0x7BC9` | `soundoff` | probable | Clears port `0x61` bits 0-1; writes `0xFF` twice to timer 0. |
| `0x7D10` | `0x7C10` | `initsound` | probable | Programs timers, sets wavetype/t0val/musvol/t2val, calls setsoundt2/soundstop/startint8. |
| `0x7DD9` | `0x7CD9` | `sett0` | verified | Writes timer 0 divisor, sets `DS:0x024A=4*t0val` (or `0xFFFF`), clamps `musvol`, calls `setsoundmode`. |
| `0x7E7C` | `0x7D7C` | `setsoundt2` | verified | `spkrmode=0`, clears `soundt0flag`, enables the speaker gate in port `0x61`. |
| `0x7EB8` | `0x7DB8` | `setsoundmode` | verified | `spkrmode=wavetype`; enables the speaker gate if needed. |
| `0x7EFF` | `0x7DFF` | `startint8` | verified | Hooks INT 8, sets `timerrate=0xFFFF`, timer 0 divisor `0x4000`, `int8flag=1`. |
| `0x7F36` | `0x7E36` | `stopint8` | verified | Restores INT 8, `sett2val(40)`, re-enables the gate, timer 0 divisor `0x8000`. |
| `0x7FA2` | `0x7EA2` | `sett2val` | verified | If `sndflag`, writes a 16-bit divisor to port `0x42`. |
| `0x7FD5` | `0x7ED5` | `timer2` / `timer2_write` | verified | Writes a 16-bit divisor to port `0x42` (LSB then MSB). |
| `0x8001` | `0x7F01` | `getlevch` | verified | Switches on level 1..8; `y*15+x`; per-level bases. |
| `0x82A6` | `0x81A6` | `main`/title loop? | probable | Calls init/score/title routines; reads keys; prints `DS:0x4FBE`. |
| `0x8C7D` | `0x8B7D` | `long_divide`? | probable | C runtime 32-bit divide helper. |
| `0x8DDD` | `0x8CDD` | `inportb`? | probable | C runtime port read helper. |
| `0x8DE8` | `0x8CE8` | `outportb`? | probable | C runtime port write helper. |
| `0x8E23` | `0x8D23` | `long_compare`? | probable | C runtime 32-bit compare helper. |
| `0x17EB` | `0x16EB` | video RLE blit? | probable | Copies `DS:0x0DAC` to `B800`; `0xFE` starts a run. |

## Data

| DS | file | Size | Name | Confidence |
|---|---:|---:|---|---|
| `0x0255` | `0x8FC5` | 14 B | CGA CRTC table R0..R13 | verified |
| `0x0263` | `0x8FD3` | text | "Please turn on display connected to your Color Graphics Display Adapter" (`$` at file `0x9091`; printable text starts file `0x9005` / DS `0x0295`) | verified |
| `0x0322` | `0x9092` | text | "This program requires the Color/Graphics Display Adapter" (`$` at file `0x9122`; printable text starts file `0x90C4` / DS `0x0354`) | verified |
| `0x0DAC` | `0x9B1C` | RLE | title bitmap / RLE data | probable |
| `0x45E4` | `0xD354` | 172 B | bonus jingle `T1` (86 words / 43 notes) | verified |
| `0x4690` | `0xD400` | 112 B | embedded default high scores | verified |
| `0x4700` | `0xD470` | 358 B | bonus jingle `T2` (179 words / 89 notes) | verified |
| `0x4866` | `0xD5D6` | 582 B | background jingle (291 words) | verified |
| `0x4AAC` | `0xD81C` | 98 B | dirge (49 words) | verified |
| `0x4B0E` | `0xD87E` | 150 B | level 1 | verified |
| `0x4BA4` | `0xD914` | 150 B | level 2 | verified |
| `0x4C3A` | `0xD9AA` | 150 B | level 3 | verified |
| `0x4CD0` | `0xDA40` | 150 B | level 4 | verified |
| `0x4D66` | `0xDAD6` | 150 B | level 5 | verified |
| `0x4DFC` | `0xDB6C` | 150 B | level 6 | verified |
| `0x4E92` | `0xDC02` | 150 B | level 7 | verified |
| `0x4F28` | `0xDC98` | 150 B | level 8 | verified |
| `0x4FBE` | `0xDD2E` | text | NUL-separated UI strings | verified |
| `0x509A` | `0xDE0A` | 257 B | C `ctype` / character-class table | verified |

## Sound/timer state (BSS, inferred)

These are data-segment variables, not tables. Names are from the port; offsets come from the disassembly.

| DS | Port name | Evidence |
|---:|---|---|
| `0x0248` | `spkrmode` | ISR tests 0/1/other. |
| `0x024A` | `timerrate` | ISR adds this to `timercount`; `sett0` stores `4*t0val` (or `0xFFFF`). |
| `0x024C` | `timercount` | ISR accumulator. |
| `0x024E` | `timerclock` | Byte incremented before `soundint`. |
| `0x024F` | `pulsewidth` | ISR PWM loop count. |
| `0x0251/0x0253` | old INT 8 vector | Saved by the hook. |
| `0x0667` | `soundflag` | `soundint` tests it. |
| `0x0669` | `musicflag` | Set together with `sndflag`. |
| `0x4566` | `sndflag` | `sett0`/`sett2val` test it. |
| `0x4568` | `int8flag` | `startint8`/`stopint8`. |
| `0x456A` | `soundt0flag` | `setsoundt2`/`setsoundmode`. |
| `0x456C` | `wavetype` | `musicupdate` writes 1. |
| `0x456E` | `t2val` | `soundint` sets 40. |
| `0x4570` | `t0val` | `soundint` sets `0x7D00`; `musicupdate` sets note. |
| `0x4572` | `musvol` | Music envelope. |
| `0x458E/0x4590/0x4592` | `soundexplodeflag/value/duration[0]` | `soundexplode` writes 1/1500/10. |
| `0x459E` | `soundemeraldduration` | `soundemerald` writes 7. |
| `0x45A0/0x45A2/0x45A4` | `soundemeraldflag/emerfreq/n` | `soundemerald` writes 1/freq/0. |
| `0x45AE/0x45B0/0x45B2` | `soundddieflag/n/value` | `soundddie` writes 1/0/20000. |
| `0x45C8` | `musicp` | Music index. |
| `0x45CA` | `tuneno` | Tune number. |
| `0x45CC` | `noteduration` | Note countdown. |
| `0x45CE` | `notevalue` | Current note. |
| `0x45D0` | `musicplaying` | Music flag. |
| `0x45D2` | `soundpaused` | `soundint` checks it. |
| `0x45D4` | `musicmaxvol` | Envelope 50. |
| `0x45D6` | `musicattackrate` | 20/50/50. |
| `0x45D8` | `musicsustainlevel` | 20/8/25. |
| `0x45DA` | `musicdecayrate` | 10/15/5. |
| `0x45DC` | `musicnotewidth` | Note width. |
| `0x45DE` | `musicreleaserate` | 4/1/1. |
| `0x45E0` | `musicstage` | 0/1/2. |
| `0x45E2` | `musicn` | Note counter. |

## Game state (BSS, inferred)

| DS | Port name | Evidence |
|---:|---|---|
| `0x03B3` | `cga_retrace_flag` | Data byte starts `0xFF`; `cga_wait_retrace` tests it. |
| `0x03B4` | `cga_write_mode` | `0` = replace, `0xFF` = XOR. |
| `0x03B5` | `cga_color_select` | Cached `3D9` value. |
| `0x03B6` | `cga_pixel_color` | Current 2-bit pixel colour. |
| `0x0673` | `cga_r2` | Current CRTC R2; data byte is `0x2C`, clamped to `0x2A..0x2E`. |
| `0x0658` | `keypressed` | `testpause` and the input code read it. |
| `0x4FE0` | lives field A | `getlives(arg==1)` / `addlife`. |
| `0x4FE2` | level, `curplayer` | `levno`: `base + 8*curplayer`. |
| `0x4FE8` | lives field B | `getlives(other)` / `addlife`. |
| `0x500C` | `curplayer` | Getter at `CS:0x8B45`. |
| `0x500E` | `nplayers`/`diggers`? | Getter at `CS:0x8B4D`. |
| `0x5014` | `penalty` | `incpenalty` (`CS:0x8A32`). |
