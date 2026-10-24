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
| `0x5FA1` | `0x5EA1` | `addscore` | verified | 32-bit per-player score; caps at 999999 (wraps to 0), draws it, grants an extra life every 20000 up to 5 lives, then three `incpenalty` calls. |
| `0x689F` | `0x679F` | `scorekill` | verified | Calls `score_add(250)`. |
| `0x68AD` | `0x67AD` | `scoreemerald` | verified | Calls `score_add(25)`. |
| `0x68BB` | `0x67BB` | `scoreoctave` | verified | Calls `score_add(250)`; called from `soundemerald` on wrap-around. |
| `0x68C9` | `0x67C9` | `scoregold` | verified | Calls `score_add(500)`. |
| `0x68D7` | `0x67D7` | `scorebonus` | verified | Calls `score_add(1000)`. |
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
| `0x76F7` | `0x75F7` | `soundemerald` | verified | The argument is only a start flag: `0` resets the scale to `0x8E8`; nonzero steps `DS:0x45A2` up the table `0474/04B8/054C/05F2/06AC/0712/07F0/08E8` and calls `scoreoctave` on wrap. Sets `duration=7`, `n=0`, `flag=1`. |
| `0x76CC` | `0x75CC` | `soundem` | verified | Sets `DS:0x459C=1`. |
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
| `0x82A6` | `0x81A6` | `main`/title | verified | Video/sound init, the 10-try disk signature check, title animation (scripted frames 0x32..0xDF), menu key wait, then the two-player life loop calling `initmbspr` + `game` per frame. |
| `0x08E5` | `0x07E5` | `drawglyph` | verified | Reads `[w][h]` at the DS pointer; draws with `gputich` and the colour byte `DS:0x0623`. |
| `0x11E4` | `0x10E4` | `drawchar` | verified | `ch-0x20` indexes the 64-entry glyph table at `CS:0x1523`; only space, `.`, digits, A-Z, `_` exist. |
| `0x11A4` | `0x10A4` | `writestring` | verified | Walks a NUL-terminated DS string, one 12-pixel cell per character. |
| `0x026C` | `0x016C` | `diskcheck` | verified | Calls the sector-2 signature check twice; returns `0x45` on success, `0` otherwise. |
| `0x02F3` | `0x01F3` | `disk_signature` | verified | Sector-2 byte `0x0E` plus BIOS `0x40:0x42..0x44` = `40 20 20`. |
| `0x0321` | `0x0221` | `read_sector2` | verified | INT 13h AH=2, cylinder 0x27, sector 2. |
| `0x08BE0` | `0x08AE0` | `hang` | verified | Infinite loop; called when the signature check fails. |
| `0x5B3F` | `0x5A3F` | `input_init` | verified | Resets the keyboard-controller/input state. |
| `0x5C5E` | `0x5B5E` | `readkey` | verified | Returns nonzero when a normal key press should start the game. |
| `0x0E06` | `0x0D06` | `install_keyboard` | verified | Saves the old keyboard vector and installs the handler. |
| `0x8C7D` | `0x8B7D` | `ldivmod` | verified | Signed 32/32 divide; quotient AX:BX, remainder DX (and high word CX). |
| `0x8DDD` | `0x8CDD` | `inportb` | verified | `in al,dx; xor ah,ah`. |
| `0x8DE8` | `0x8CE8` | `outportb` | verified | `out dx,al`. |
| `0x8E23` | `0x8D23` | `lcmp` | verified | Signed 32-bit compare; callers use the flags (`jl`/`jge`). |
| `0x0172` | `0x0072` | `__exit` | verified | Restores SP from `DS:0x0000` and far-returns to DOS. |
| `0x017B` | `0x007B` | `parse_cmdline` | verified | Skips spaces and fills the argv array at `DS:0x0004`. |
| `0x0227` | `0x0127` | `iabs` | verified | Integer absolute value. |
| `0x80F5` | `0x7FF5` | `digit_value` | verified | `'1'..'9'` → 1..9, otherwise 0. |
| `0x8155` | `0x8055` | `toupper_c` | verified | Lowercase to uppercase. |
| `0x8BFC` | `0x8AFC` | `lmul` | verified | Signed 32x32 multiply; only the low 32 bits are kept. |
| `0x8C6D` | `0x8B6D` | `exit_process` | verified | Pushes the status and tails into `__exit`. |
| `0x8D4C` | `0x8C4C` | `ultoa` | verified | Unsigned long to decimal with a minimum field width. |
| `0x17EB` | `0x16EB` | video RLE blit? | probable | Copies `DS:0x0DAC` to `B800`; `0xFE` starts a run. |

### Game loop, RNG, score storage

| CS | file | Name | Confidence | Notes |
|---|---:|---|---|---|
| `0x028F` | `0x018F` | `read_sector7` | verified | INT 13h AH=2: drive 0, head 0, cylinder 0x27, sector 7, 1 sector, buffer `DS:0x0046`. |
| `0x02B8` | `0x01B8` | `write_sector7` | verified | INT 13h AH=4 verify then AH=3 write; same sector; 3 retries. |
| `0x0321` | `0x0221` | `read_sector2` | verified | Disk signature / copy-protection check. |
| `0x02F3` | `0x01F3` | `disk_check` | verified | Reads sector 2 and checks the BIOS/disk signature. |
| `0x177F` | `0x167F` | `rand` | verified | Shared game/sound RNG: table `DS:0x0CA7`, counter `DS:0x0DA7`, BIOS tick `0x40:0x006C`. |
| `0x17C1` | `0x16C1` | `bios_tick_byte` | verified | Reads the byte at `0x40:0x006C`. |
| `0x1A03` | `0x1903` | `initmbspr` | verified | Sets default 4x15 frames for sprites 1..7 (`0x18D6/0x1913`) and 8..13 (`0x1DD0/0x1E0D`), then `initdbfspr`. |
| `0x1826` | `0x1726` | `creatembspr` | verified | Sets a third frame per sprite (1..7 and 8..13), calls `createdbfspr`, resets the six monster leg counters. |
| `0x1E9D` | `0x1D9D` | `drawgold` | verified | Frames 0..6, 4x15, seven source pairs; returns the collision mask. |
| `0x263F` | `0x253F` | `drawdig` | verified | Global leg animation (0x2700/0x2702), 4 directions x 3 legs x fire/no-fire, death frames 10..15; sprite 0. |
| `0x2C90` | `0x2B90` | `domonsters` | verified | Six monsters, stride 0x28, `hnt > 10-levof10()`, `randno(15-levof10())`, calls `monai`. |
| `0x2E75` | `0x2D75` | `monai` | verified | Whole update: direction selection, wall stop, `eatfield`, blob drawing and movement, `hitemerald`, `stime`, `hnt`, `drawmon`, bag/monster/digger collisions, coordinate update. Digger 0 is chased directly (no per-monster `chase` field). |
| `0x2BC1` | `0x2AC1` | `initmonsters` | verified | Clears flags; `mongaptime=45-2*levof10()`, `totalmonsters=levof10()+5`, `maxmononscr` 3/4/5, `nextmontime=10`, `unbonusflag=1`. |
| `0x2C58` | `0x2B58` | `erasemonsters` | verified | `erasespr(i+8)` for each flag-set monster. |
| `0x2DD3` | `0x2CD3` | `createmonster` | verified | Fills the record (`x=292`, `y=18`, `dir=hdir=LEFT`, `stime=5`) and calls `movedrawspr`. |
| `0x3682` | `0x3582` | `mondie` | verified | Death states 1/4, `bagy`, `drawmondie`, `getbagdir`, `killmon`, `scorekill`. |
| `0x3767` | `0x3667` | `collision_penalties` | verified | `incpenalty()` once per colliding monster bit. |
| `0x3795` | `0x3695` | `monster_collisions` | verified | Reverses a colliding monster's direction when both have `stime==0` and equal `dir`. |
| `0x3815` | `0x3715` | `fieldclear` | verified | Field tests for the four directions; matches the port. |
| `0x3ABB` | `0x39BB` | `nmononscr` | verified | Counts flag-set monsters. |
| `0x3AEF` | `0x39EF` | `incmont` | verified | Increments `t` for monsters 1..n-1 (`n` clamped to 6). |
| `0x3B26` | `0x3A26` | `mongold` | verified | Sets `DS:0x3C0C`. |
| `0x1B84` | `0x1A84` | `drawmon` | verified | Per-monster leg animation (0x16CC/0x16D8); hobbin set plus nobbin right/left sets; sprite `mon+8`; returns the mask. |
| `0x1DDB` | `0x1CDB` | `drawmondie` | verified | Hobbin 4x15, nobbin right 4x15, nobbin left 4x14; sprite `mon+8`. |
| `0x260E` | `0x250E` | `drawbonus` | verified | Sets sprite `0x0E` (4x15) and `movedrawspr(0x0E,x,y)`. |
| `0x208A` | `0x1F8A` | `drawemerald` | verified | 4x10 misc sprite, sources `0x16E4/0x170D`. |
| `0x20C0` | `0x1FC0` | `eraseemerald` | verified | 4x10 misc sprite, sources `0x175E/0x1787`. |
| `0x22E8` | `0x21E8` | `drawfurryblob` | verified | `x-4, y+15`, 6x8, sources `0x282E/0x285F`. |
| `0x2334` | `0x2234` | `drawsquareblob` | verified | `x-4, y+17`, 6x6, sources `0x2890/0x28B5`. |
| `0x237A` | `0x227A` | `drawbackg` | verified | Tiles 5x4 over `y=14..199`, `x=0..319`; eight tile source pairs selected by plan. |
| `0x24C6` | `0x23C6` | `drawfire` | verified | Sprite `0x0F`, 2x7: frame 0 animates three legs, frames 1..3 fixed explosion frames; returns the mask. |
| `0x20F6` | `0x1FF6` | `createdbfspr` | verified | Three-frame buffers for the digger/bonus/fireball and resets their animation state. |
| `0x2167` | `0x2067` | `initdbfspr` | verified | One-frame defaults for the digger/bonus/fireball and resets their animation state. |
| `0x0754` | `0x0654` | `setspr2` | verified | `(spr, src0, src1, src2, w, h, a, b)`; also fills the live arrays and clears `sprenf`. |
| `0x07E4` | `0x06E4` | `drawspr` | verified | Sets `0x59F/0x5A1`, then falls into `0x0997`. |
| `0x0997` | `0x0897` | `drawspr` core | verified | Save/restore the live box, dirty recursion, `putis`, `ggeti`, `putims`, `bcollides`. |
| `0x0804` | `0x0704` | `setspr` | verified | `(spr, src0, src1, w, h, a, b)`. |
| `0x063C` | `0x053C` | `setorder` | verified | Draw-order pointer; 0 selects the default table at `0x0627`. |
| `0x084F` | `0x074F` | `initmiscspr` | verified | `(x, y, w, h)` for the misc slot; calls `0x0A32`. |
| `0x0881` | `0x0781` | `initmiscspr2` | verified | As above with `w/4+1`; returns a 16-bit redraw mask (`0x0A3C`). |
| `0x08AD` | `0x07AD` | `drawmiscspr` | verified | `(x, y, src0, src1, w, h)`; calls `0x0CF3`. |
| `0x0A65` | `0x0965` | `clearrdrwf` | verified | `clearrecf()` + clear the 17 `sprrdrwf` words. |
| `0x0A7C` | `0x097C` | `clearrecf` | verified | Clears the 17 `sprrecf` words. |
| `0x0A90` | `0x0990` | `setrdrwflgs` | verified | Recursive overlap marking; retries with +320/-2 and -640/+4 edge shifts. |
| `0x0B05` | `0x0A05` | `collide` | verified | Box overlap test (width << 2). |
| `0x0B6E` | `0x0A6E` | `bcollide` | verified | Collision-box overlap test. |
| `0x0D48` | `0x0C48` | `bcollides` | verified | Fills `first[5]`/`coll[16]` using the edge-wrapped `bcollide`. |
| `0x0D30` | `0x0C30` | `putis` | verified | Restores every dirty sprite's saved background. |
| `0x0D13` | `0x0C13` | `putims` | verified | Draws dirty sprites in the order-table order. |
| `0x0CD2` | `0x0BD2` | `ggeti` | verified | Screen → saved buffer, row by row. |
| `0x0BFF` | `0x0AFF` | `gputi` | verified | Saved buffer → screen. |
| `0x0CF3` | `0x0BF3` | `gputim` | verified | Masked draw: `(screen & mask) | data`. |
| `0x0C1E` | `0x0B1E` | `gputich` | verified | Glyph draw using `DS:0x0623`. |
| `0x0876` | `0x0776` | `getis` | verified | `ggeti` for dirty sprites, then `putims`. |
| `0x6C3E` | `0x6B3E` | `drawfield` | verified | Draws wall pieces from the field bits; matches the port's `drawfield()`. |
| `0x6BCA` | `0x6ACA` | `savefield` | verified | Copies the working field to the per-player saved copy. |
| `0x21CC` | `0x20CC` | `drawrightblob` | verified | `x+16, y-1`, 2x18. |
| `0x2214` | `0x2114` | `drawleftblob` | verified | `x-8, y-1`, 2x18. |
| `0x225C` | `0x215C` | `drawtopblob` | verified | `x-4, y-6`, 6x6. |
| `0x22A2` | `0x21A2` | `drawbottomblob` | verified | `x-4, y+15`, 6x6. |
| `0x39A9` | `0x38A9` | `killmon` | verified | Clears flag/alive, `erasespr(mon+8)`. |
| `0x569B` | `0x559B` | `digresettime` | verified | `bagtime=0`. |
| `0x5DD8` | `0x5CD8` | `getfirepflag` | verified | Returns `DS:0x40BC`. |
| `0x56A6` | `0x55A6` | `ggetpix` | verified | Reads a 2-bit CGA pixel at `(x,y)`. |
| `0x023F` | `0x013F` | `delay` | verified | Busy loop, `n*100` iterations. |
| `0x5B3F` | `0x5A3F` | `input_init` | verified | Mouse/joystick detection and reset of the direction/fire state. |
| `0x5C5E` | `0x5B5E` | `readkey` | verified | Returns nonzero to start the game; ignores scan code 1 and break codes. |
| `0x0DBB` | `0x0CBB` | `sample_mouse` | verified | Port 0x201: bit 0 = X count, bit 1 = Y count, bits 4/5 = buttons. |
| `0x0E06` | `0x0D06` | `install_keyboard` | verified | Hooks INT 9, saves the old vector at `DS:0x0647/0x0649`. |
| `0x0E2F` | `0x0D2F` | `restore_keyboard` | verified | Restores the saved INT 9 vector. |
| `0x0E48` | `0x0D48` | `keyboard_isr` | verified | Scan code, port 0x61 acknowledge, 21-entry dispatch, EOI. |
| `0x72D7` | `0x71D7` | `soundstopall` | verified | Calls the 13 effect/music off routines. |
| `0x7807` | `0x7707` | `soundgold` | verified | duration 500, freq 4000, n 30, flag 1. |
| `0x7430` | `0x7330` | `soundfall` | verified | value 1000, flag 1. |
| `0x7559` | `0x7459` | `soundwobble` | verified | flag 1. |
| `0x7891` | `0x7791` | `soundeatm` | verified | duration 20, n 3, value 2000, flag 1. |
| `0x76CC` | `0x75CC` | `soundem` | verified | flag 1. |
| `0x7646` | `0x7546` | `soundbonus` | verified | flag 1. |
| `0x7A63` | `0x7963` | `musicoff` | verified | Clears `musicp` and `musicplaying`. |
| `0x7310` | `0x7210` | `soundlevdone` | verified | Blocking 11-note level-done tune driven by `DS:0x024E`. |
| `0x5598` | `0x5498` | `sceatm` | verified | `addscore(msc*200)`, then `msc <<= 1` (`DS:0x4014`). |
| `0x5668` | `0x5568` | `killdigger` | verified | Ignore if already in death states 2/3/4; `musicoff`, alive=0, store death/bag. |
| `0x57F1` | `0x56F1` | `hitemerald` | verified | Uses `embox[8]={8,12,12,9,16,12,6,9}`; draws at `embox[dir]`, erases + returns hit at `embox[dir+1]`; emerald map bytes at `DS:0x4018`. |
| `0x5AE0` | `0x59E0` | `killemerald` | verified | Clears the emerald at `(h, v+1)` and erases its sprite. |
| `0x5A94` | `0x5994` | `countem` | verified | Counts emeralds selected by `emmask`. |
| `0x3A40` | `0x3940` | `killmonsters` | verified | `killmon()` over the monster bits of the mask; returns the count. |
| `0x3A7C` | `0x397C` | `squashmonster` | verified | `alive=0`, `death`, `bag`. |
| `0x39E7` | `0x38E7` | `squashmonsters` | verified | Squashes colliding monsters whose `y >= bagy(bag)`. |
| `0x3962` | `0x3862` | `checkmonscared` | verified | Monsters on row `h` facing UP turn DOWN. |
| `0x3AA5` | `0x39A5` | `monleft` | verified | `nmononscr() + totalmonsters - nextmonster`. |
| `0x5602` | `0x5502` | `checkdiggerunderbag` | verified | True when the vertical-moving digger is on the bag's column and row `v` or `v-1`. |
| `0x689F` | `0x679F` | `scorekill` | verified | `addscore(250)`. |
| `0x6DFE` | `0x6CFE` | `eatfield` | verified | Clears the sub-cell bits for the direction (horizontal 0x1F, vertical 0xFC0) and 0x2000 when the cell empties. Field words at `DS:0x4438`, row stride 0x1E. |
| `0x7891` | `0x7791` | `soundeatm` | verified | flag 1, duration 20, value 0x7D0, n 3. |
| `0x3AEF` | `0x39EF` | `incmont` | probable | Called with `penalty-8`. |
| `0x3B31` | `0x3A31` | `initbags` | verified | `pushcount=0`, `goldtime=150-10*levof10()`, clears `exist`, scans `getlevch(x,y,levplan())=='B'` (x 0..14, y 0..9) and fills the records, then copies to the saved array. |
| `0x3C93` | `0x3B93` | `drawbags` | verified | Restores the saved bag array and `movedrawspr(bag,...)`. |
| `0x3E2D` | `0x3D2D` | `dobags` | verified | Seven bags, stride 0x28; gold frames 4/5/6, `removebag`, field check, fall/wobble sound flags. |
| `0x3FED` | `0x3EED` | `updatebag` | verified | dir NONE/LEFT/RIGHT/DOWN wobble-fall state machine, `getfield`/`checkdiggerunderbag`, `baghitground`, `pushcount`/`pushbag` tail. |
| `0x4296` | `0x4196` | `baghitground` | verified | `fallh>1` → `gt=1`; resets dir/wt/wobbling; draws frame 0; removes collided bags. |
| `0x433B` | `0x423B` | `pushbag` | verified | Gold early-out, falling-bag kill, edge stop, ±4/+6 move, unfallen blobs, `eatfield`/`killemerald`, horizontal push chain, digger/monster undo, coordinate update. |
| `0x466C` | `0x456C` | `pushbags` | verified | `pushbag(bag,dir)` over the bag bits; all must succeed. |
| `0x46B2` | `0x45B2` | `pushudbags` | verified | Vertical push: only gold bags succeed (`getgold`); otherwise false. |
| `0x4703` | `0x4603` | `removebag` | verified | Clears `exist` and erases the sprite. |
| `0x4730` | `0x4630` | `bagy` | verified | Bag `y`. |
| `0x4749` | `0x4649` | `getbagdir` | verified | Bag `dir`, or `-1` when absent. |
| `0x4774` | `0x4674` | `bagmask` | verified | OR of `1<<bag` for existing bags. |
| `0x47B5` | `0x46B5` | `removebags` | verified | `removebag()` for the bags in the mask. |
| `0x47F8` | `0x46F8` | `getnmovingbags` | verified | Counts `exist`, `gt<10`, and (`gt!=0` or `wobbling`). |
| `0x4853` | `0x4753` | `getgold` | verified | Draws frame 6; on a digger hit scores/sounds/resets bagtime, else `mongold()`; removes the bag. |
| `0x48A0` | `0x47A0` | `initdigger` | verified | Full single-digger reset (`h=7`, `v=9`, `x=152`, `y=180`, dirs LEFT/RIGHT, flags, `movedrawspr(0)`). |
| `0x4939` | `0x4839` | `dodigger` | verified | Per-frame digger update; calls `drawdig`/`updatefire`/`drawexplosion`/`updatedigger`/`diggerdie`; contains the bonus-mode block at `CS:0x4912..0x4A11`. |
| `0x4A3E` | `0x493E` | `updatefire` | verified | Launch (`levof10()*3+60` recharge, `fx/fy` per dir, sprite 15), pixel probes in 7-pixel steps, `drawfire`, monster hits (`killmon`/`scorekill`/`expsn=1`), `0x40FE` impact test, edge/impact explosion. |
| `0x4E66` | `0x4D66` | `drawexplosion` | verified | `expsn` 1..3 draw frame 1..3 + `soundexplode` on 1, then `killfire` and `expsn=0`. |
| `0x4EE3` | `0x4DE3` | `killfire` | verified | If firing, sets `notfiring`, `erasespr(15)`, `soundfireoff`. |
| `0x07B1` | `0x06B1` | `movedrawspr` | verified | Sprite `spr&0xF`: stores masked `x`, `y`, calls draw routine `0x935`. |
| `0x07D3` | `0x06D3` | `erasespr` | verified | Sprite `spr&0xF`: calls erase routine `0x984`. |
| `0x24C6` | `0x23C6` | `drawfire` | verified | `drawfire(x, y, frame)`; returns the sprite collision mask. |
| `0x56A6` | `0x55A6` | `ggetpix` | verified | CGA pixel read: `(y&~1)*40 + (y&1)*0x2000`, 2-bit pixel. |
| `0x5DD8` | `0x5CD8` | `getfirepflag` | verified | Returns `DS:0x40BC`. |
| `0x7571` | `0x7471` | `soundfire` | verified | Value 500, flag 1. |
| `0x7582` | `0x7482` | `soundfireoff` | verified | Clears the fire sound flag/value. |
| `0x75E6` | `0x74E6` | `soundexplode` | verified | Value 1500, duration 10, flag 1; calls `soundfireoff`. |
| `0x4F07` | `0x4E07` | `updatedigger` | verified | `readdir`/`getdir`, input gating by `rx`/`ry`, edge stop, `eatfield`, blob move, `hitemerald` (score/sound/cooldown), `drawdig`, bag push, bonus-mode monster kills, bonus pickup, coordinate update. |
| `0x5218` | `0x5118` | `diggerdie` | verified | Death states 1..5: bag settle, animation frames 10..14, fall arc 3/5/6/6/5/3/0, `setdead(1)`. Life loss/respawn are handled by the main loop, not here. |
| `0x55B5` | `0x54B5` | `bonusmode_get` | verified | Returns `DS:0x4012`. |
| `0x55BD` | `0x54BD` | `reversedir` | verified | RIGHT<->LEFT, UP<->DOWN, others unchanged. |
| `0x5650` | `0x5550` | `diggerx` | verified | Returns `DS:0x3FDE`. |
| `0x5658` | `0x5558` | `diggery` | verified | Returns `DS:0x3FE0`. |
| `0x5660` | `0x5560` | `digalive` | verified | Returns `DS:0x3FF2`. |
| `0x56E8` | `0x55E8` | `makeemfield` | verified | `emmask` 1/2; sets/clears the emerald bit per `getlevch(x,y,levplan())=='C'`. |
| `0x5777` | `0x5677` | `drawemeralds` | verified | Sets `emmask`, draws every selected emerald. |
| `0x6A48` | `0x6948` | `makefield` | verified | Fills `0xFFFF`, clears `0xD03F` for `S/V` and `0xDFE0` for `S/H`, saves to the per-player copy. |
| `0x6B31` | `0x6A31` | `drawstatics` | verified | Restores the saved field, `setretr(1)`, `gpal(0)`, `ginten(0)`, `drawbackg(levplan())`, `drawfield()`. |
| `0x7441` | `0x7341` | `soundfalloff` | probable | Called at the end of `dobags`. |
| `0x7497` | `0x7397` | `soundbreak` | probable | Sets flag/duration 3/freq `0x3A98`; called by `dobags`. |
| `0x7564` | `0x7464` | `soundwobbleoff` | probable | Called at the end of `dobags`. |
| `0x7646` | `0x7546` | `soundbonus` | probable | Called by `dodigger`. |
| `0x7651` | `0x7551` | `soundbonusoff` | probable | Called by `dodigger`. |
| `0x5510` | `0x5410` | `createbonus` | verified | `bonusvisible=1`, draw bonus sprite. |
| `0x5528` | `0x5428` | `initbonusmode` | verified | `250 - levof10()*20`, `startbonustimeleft=20`, `bonusmode=1`. |
| `0x5562` | `0x5462` | `endbonusmode` | verified | `bonusmode=0`, ginten(0). |
| `0x5575` | `0x5475` | `erasebonus` | verified | If visible, erase sprite 0x0E; ginten(0). |
| `0x5A94` | `0x5994` | `countem` | probable | Level-completion check. |
| `0x5BB9` | `0x5AB9` | `readdir` | verified | Merges the key edge/hold directions, sets `firepflag`, samples the mouse when active. |
| `0x5D71` | `0x5C71` | `getdir` | verified | Returns `DS:0x40B0`, or `-1` for pause. |
| `0x5DE0` | `0x5CE0` | `initscores` | verified | Calls `addscore(0)`. |
| `0x5DED` | `0x5CED` | `loadscores` | verified | Reads sector 7, parses `s` + ten `[3 initials][2 pad][6 digits]` entries into `DS:0x4134`/`0x4108`; falls back to zeroed entries when the header is wrong. |
| `0x6521` | `0x6421` | `savescores` | verified | Builds the same `s`+ten-entry 512-byte buffer and writes sector 7. |
| `0x71D7` | `0x70D7` | `soundstop` | probable | Post-level sound stop. |
| `0x7310` | `0x7210` | `soundlevdone` | probable | Level-done tune. |
| `0x7A63` | `0x7963` | `musicoff` | probable | Music off. |
| `0x86F1` | `0x85F1` | `game` | verified | Runs a whole life/level: signature re-check; on a new level `drawscreen`, optional `PLAYER n` banner (15 colour flashes); frame loop `readdir`/`dodigger`/`domonsters`/`dobags`/penalty/`testpause`/`checklevdone`; on death/level end a 20-frame bag-settle loop, then `playmusic(0)`, `killfire`, `erasebonus`, `cleanupbags`, `savefield`, `erasemonsters`, level advance (countem/levdone) and life loss. |
| `0x8A3B` | `0x893B` | `cleartopline` | verified | Writes 26 spaces at (0,0) and one at (0x134,0). |
| `0x5F2E` | `0x5E2E` | `drawscore` | verified | Draws the current player score at (0,6) in the given colour. |
| `0x62D0` | `0x61D0` | `drawhighscore` | verified | Draws the high score at (0,6), colour 3. |
| `0x610B` | `0x600B` | `gameover` | verified | `addscore(0)`, compares the score with the tenth entry; on a new high score it clears, prompts for initials and saves; otherwise the flashing `GAME OVER` display. |
| `0x201A` | `0x1F1A` | `drawlife` | verified | 4x12 icon; frames 0/1/2 use `0x17B0`, `0x1812`, `0x1874` source pairs. |
| `0x6345` | `0x6245` | `drawlives` | verified | Up to four icons per player at `i*20+0x3C` (player 1) or `0xF4-i*20` (player 2). |
| `0x6407` | `0x6307` | `draw_highscores` | verified | `ONE`/`TWO` plus ` PLAYER `/` PLAYERS`. |
| `0x646E` | `0x636E` | `draw_menu` | verified | `HIGH SCORES` and the ten-entry table at `i*13+0x1F`. |
| `0x65B5` | `0x64B5` | `enter_initials` | verified | Prompts for three initials after a new high score. |
| `0x6807` | `0x6707` | `insert_score` | verified | Shifts the table down from entry 10 to the insertion point. |
| `0x68E5` | `0x67E5` | `format_score` | verified | 32-bit decimal with leading-zero suppression. |
| `0x69C3` | `0x68C3` | `atoi_ds` | verified | Decimal string to 32-bit. |
| `0x8984` | `0x8884` | `menu_redraw` | verified | Toggles `nplayers` between 1 and 2. |
| `0x4926` | `0x4826` | `erasediggers` | verified | `erasespr(0)` and `digvisible=0`. |
| `0x3D2F` | `0x3C2F` | `cleanupbags` | verified | `soundfalloff`, removes bags at the digger start/off-grid, saves per-player copies. |
| `0x8D2D` | `0x8C2D` | `strcpy_ds` | verified | C runtime `strcpy` over data-segment offsets. |
| `0x8DF4` | `0x8CF4` | `strcat_ds` | verified | C runtime `strcat` over data-segment offsets. |
| `0x89A4` | `0x88A4` | `initlevel`? | probable | `levdone=0`, `makefield` `0x6A48`, `makeemfield` `0x56E8`, `initbags` `0x3B31`, `levnotdrawn=1` (`DS:0x5010`). |
| `0x89CD` | `0x88CD` | `drawscreen`? | probable | `creatembspr` `0x1826`, `drawstatics` `0x6B31`, `drawbags` `0x3C93`, `drawemeralds` `0x5777`, `initdigger` `0x48A0`, `initmonsters` `0x2BC1`. |
| `0x89E4` | `0x88E4` | `initchars`? | probable | `initmbspr` `0x1A03`, `initdigger` `0x48A0`, `initmonsters` `0x2BC1`. |
| `0x89F2` | `0x88F2` | `checklevdone`? | probable | Sets the per-player levdone flag. |

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
| `0x509A` | `0xDE0A` | 257 B | C `ctype` / character-class table | verified |
| `0x4FBE` | `0xDD2E` | 32 B | title sprite draw order: `14,13,7,6,5,4,3,2,1,12,11,10,9,8,15,0` | verified |
| `0x5018` | `0xDD88` | text | `"D I G G E R"` title string | verified |
| `0x5024` | `0xDD94` | text | twelve-space menu clear line | verified |
| `0x5031..0x5065` | `0xDDA1..0xDDD5` | text | `NOBBIN`, `HOBBIN`, `DIGGER`, `GOLD`, `EMERALD`, `BONUS`, `PLAYER `, `1`, `2` | verified |
| `0x06B6..0x0C80` | `0x9426..0x99F0` | 64 glyphs | title font: `[w][h][2bpp rows]`, 39-byte slots | verified |
| `0x4438` | `0xD1A8` | 300 B | play field, 15x10 16-bit cells (row stride 0x1E) | verified |
| `0x4018` | `0xCD88` | 150 B | emerald map, 15x10 bytes (row stride 0x0F) | verified |
| `0x40AE` | `0xCE1E` | 2 B | `emmask`, per-player emerald mask | verified |
| `0x3C1A` | `0xC98A` | 7x0x28 | saved bag array, player 0 | verified |
| `0x3D5A` | `0xCACA` | 7x0x28 | saved bag array, player 1 | verified |
| `0x03CF` | `0x913F` | 17 words | `sprrdrwf` needs-redraw flags | verified |
| `0x03F1` | `0x9161` | 17 words | `sprrecf` recursion guard | verified |
| `0x0413` | `0x9183` | 16 words | `sprenf` on-screen flags | verified |
| `0x0433..0x0603` | `0x91A3..0x9373` | 16 words each | live/nominal sprite sources, boxes and sizes | verified |
| `0x0625` | `0x9395` | 1 word | draw-order pointer (default `0x0627`) | verified |
| `0x0627` | `0x9397` | 16 words | default draw order `0..15` | verified |

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

## Game-body records (BSS, inferred)

The original keeps the single digger's state in globals and the bags/monsters
in arrays. Port names are given where a counterpart exists.

Digger globals:

| DS | Port name | Evidence |
|---:|---|---|
| `0x3FDE` | `x` | Position; `initdigger` sets `h*20+12`. |
| `0x3FE0` | `y` | Position; `initdigger` sets `v*18+18`. |
| `0x3FE2` | `h` | Field coordinates. |
| `0x3FE4` | `v` | Field coordinates. |
| `0x3FE6` | `rx` | Sub-cell remainder; gates vertical input. |
| `0x3FE8` | `ry` | Sub-cell remainder; gates horizontal input. |
| `0x3FEA` | `mdir` | Movement direction. Set to `-1` when no key is down. |
| `0x3FEC` | `ddir` | Key direction (port local); `updatedigger` switch writes it. |
| `0x3FEE` | `dir` | Last valid direction; used by `updatefire` for the launch offset. |
| `0x3FF0` | `bagtime` | Bag push time. |
| `0x3FF2` | `alive` | Alive flag; `initdigger` sets 1. |
| `0x3FF4` | `digvisible` | Draw flag; `initdigger` sets 1. |
| `0x3FF6` | `notfiring` | No-shot flag. |
| `0x3FF8` | `rechargetime` | Fire-recharge timer. |
| `0x3FFA` | `fx` | Fireball position. |
| `0x3FFC` | `fy` | Fireball position. |
| `0x3FFE` | `fdir` | Fireball direction. |
| `0x4000` | `expsn` | Explosion state/frame. |
| `0x4002` | `deathstage` | `initdigger` sets 1; `killdigger` writes the death state. |
| `0x4004` | `deathbag` | Bag index written by `killdigger`. |
| `0x400A` | `emn` | Emerald counter; `initdigger` clears. |
| `0x4016` | `emocttime` | Emerald-octave timer. |
| `0x0663` | `firepressed` | Cleared by `initdigger`; set to `-1` by input (`CS:0x0FF0`) and consumed at `CS:0x5BDF`. Distinct from `firepflag`, which `getfirepflag` reads at `DS:0x40BC`. |

Bag array (stride `0x28`, 7 entries):

| DS | Port name | Evidence |
|---:|---|---|
| `0x3E9A` | `x` | Position. |
| `0x3E9C` | `y` | Position. |
| `0x3E9E` | `h` | Field coordinates. |
| `0x3EA0` | `v` | Field coordinates. |
| `0x3EA2` | `xr` | Sub-cell remainder. |
| `0x3EA4` | `yr` | Sub-cell remainder. |
| `0x3EA6` | `dir` | Direction. |
| `0x3EA8` | `wobbling` | Wobble flag. |
| `0x3EAA` | `wt` | Wobble timer (15 initially, countdown; `%8` selects the frame). |
| `0x3EAC` | `unfallen` | Cleared the first time a bag is pushed down. |
| `0x3EAE` | `exist` | Live flag; `removebag` clears it. |
| `0x3EB0` | `gt` | Gold time (4/5/6) compared by `dobags`. |
| `0x3EB2` | `fallh` | Fall height; `>1` on landing makes the bag gold. |

Saved copies of the whole array live at `0x3C1A` (player 0) and `0x3D5A`
(player 1); `initbags` writes the active array plus one copy, and `drawbags`
copies it back.

Monster array (stride `0x28`, 6 entries):

| DS | Port name | Evidence |
|---:|---|---|
| `0x3B1C` | `flag` | Presence flag tested by `domonsters`. |
| `0x3B1E` | `monx` | Read by `monai`. |
| `0x3B20` | `mony` | Read by `monai`. |
| `0x3B22` | `monh` | Field coordinates used in `fieldclear`. |
| `0x3B24` | `monv` | Field coordinates used in `fieldclear`. |
| `0x3B26` | `xr` | Sub-cell remainder; nonzero enters the movement branch. |
| `0x3B28` | `yr` | Sub-cell remainder. |
| `0x3B2A` | `mondir` | Direction. |
| `0x3B2C` | `hdir` | Second direction, passed together with `dir` in the movement calls. |
| `0x3B2E` | `mont` | Freeze timer decremented by `domonsters`. |
| `0x3B30` | `alive` | Alive flag. |
| `0x3B32` | `nob` | "No bonus" flag. |
| `0x3B34` | `hnt` | Hunt timer. |
| `0x3B36` | `death` | Death state: set to 4, tested by the death path. |
| `0x3B38` | `bag` | Bag index used by the death path. |
| `0x3B3A` | `dtime` | Death timer: set to 1, then decremented. |
| `0x3B3C` | `stime` | Set to 5 by the monster-creation routine, then decremented. |

Collision mask returned by `drawmon`/`drawdig`/`drawfire`: bit 0 = digger,
bits 1..7 = bags, bits 8..13 = monsters, bit 14 = bonus, bit 15 = fireball.
The port's `clfirst[]`/`clcoll[]` type indices use a different order.

Other:

| DS | Port name | Evidence |
|---:|---|---|
| `0x3C0C` | `mongotgold` | Set by `mongold` (`CS:0x3B26`); clears a monster's time penalty. |
| `0x3C0E` | `nextmonster` | `domonsters` spawn check; incremented by `createmonster`. |
| `0x3C10` | `totalmonsters` | `levof10()+5` from `initmonsters`. |
| `0x3C12` | `maxmononscr` | 3 / 4 / 5 from `initmonsters`. |
| `0x3C14` | `nextmontime` | Spawn countdown; reset to `mongaptime`. |
| `0x3C16` | `mongaptime` | `45 - 2*levof10()`. |
| `0x3C18` | `unbonusflag` | Set by `initmonsters`, cleared when `createbonus` is called. |
| `0x4002` | `digdeath` | `killdigger` death state. |
| `0x4004` | `digbag` | `killdigger` bag index. |
| `0x3FDA` | `pushcount` | Set to 1 after a push; decremented per frame. |
| `0x3FDC` | `goldtime` | `150 - levof10()*10` from `initbags`; bag `gt` is compared with it. |
| `0x5010` | `levnotdrawn` | Set to 1 by `initlevel`. |

## Score / RNG / bonus state (BSS, inferred)

| DS | Port name | Evidence |
|---:|---|---|
| `0x0046` | `scorebuf` / disk sector buffer | 512 bytes; INT 13h buffer; parsed by `loadscores`. |
| `0x0247` | disk retry counter | `read/write_sector7` retries 3 times. |
| `0x0CA7` | `rng_table` | 256-byte permutation used by `rand`. |
| `0x0DA7` | `rng_counter` | Incremented by `rand`; starts at 0. |
| `0x400C` | `startbonustimeleft` | 20 in `initbonusmode`. |
| `0x400E` | `bonustimeleft` | `250 - levof10()*20`. |
| `0x4010` | `bonusvisible` | `createbonus` / `erasebonus`. |
| `0x4012` | `bonusmode` | `initbonusmode` / `endbonusmode`. |
| `0x4014` | `bonus_msc`? | Set to 1 by `initbonusmode`; port sets `digdat[i].msc=1`. |
| `0x40D6` | `highbuf` | 6-byte score text used by `loadscores`. |
| `0x4108` | `scorehigh` | 12 x 32-bit scores. |
| `0x4134` | `scoreinit` | 11 x 4-byte initials. |
| `0x4690` | default score block | 112-byte default/legacy image; no direct code reference. |
