# CGA video analysis — Digger and the DOSBox garbling

## Short version

The game does **not** use an exotic resolution. It uses **standard CGA mode 4:
320×200, 4 colours, 2 bits/pixel, interleaved scanlines**. The "undocumented"
part is how it gets there: it programs the CGA registers directly instead of
calling INT 10h, and it uses the **PCjr horizontal-sync value `R2=0x2C`**
(IBM CGA mode 4 uses `0x2D`). It also rewrites `R2` at runtime for a small
horizontal shift/scroll.

That is enough to break DOSBox's default EGA/VGA machine, and possibly its CGA
machine if the direct CRTC writes are not reflected in the renderer's timing.

## The exact mode setup

Entry at `CS:0x03F4` (`file 0x02F4`) calls `CS:0x03FC`, which:

1. probes CGA memory by writing/reading `B800:0000` (`CS:0x0418`);
2. if present, calls `CS:0x0429` (`file 0x0329`);
3. prints a startup message through INT 10h.

`CS:0x0429` does this:

```
out 3D8h, 02h              ; video off, graphics, colour, 320x200
CRTC R0..R13 = 38 28 2C 0A 7F 06 64 70 02 01 20 00 00 00
out 3D9h, 20h              ; background black, palette 1
fill 16KB framebuffer with colour 0
out 3D8h, 0Ah              ; video on, graphics, colour, 320x200
```

`main` then calls `gpal(0)` (`CS:0x04E7`) and `gbackground(0)`
(`CS:0x0520`), so the game itself runs in **palette 0**: black, green, red,
brown.  The `0x20` above only selects palette 1 momentarily during init.

`0x02`/`0x0A` mean: bit 1 = graphics, bit 2 = colour, bit 3 = video enable,
bit 4 = 320×200 (not 640×200), bit 5 = blink off.

### CRTC table

| Reg | Value | Meaning | IBM CGA mode 4 |
|---|---:|---|---:|
| R0 | `0x38` | horizontal total (56 chars) | `0x38` |
| R1 | `0x28` | horizontal displayed (40 chars = 320 px) | `0x28` |
| R2 | `0x2C` | **H-sync position — PCjr value** | `0x2D` |
| R3 | `0x0A` | H-sync width | `0x0A` |
| R4 | `0x7F` | vertical total (128 rows) | `0x7F` |
| R5 | `0x06` | vertical total adjust | `0x06` |
| R6 | `0x64` | vertical displayed (100 rows × 2 lines = 200) | `0x64` |
| R7 | `0x70` | V-sync position | `0x70` |
| R8 | `0x02` | interlace mode (normal) | `0x02` |
| R9 | `0x01` | max scan line (2 scanlines/row) | `0x01` |
| R10 | `0x20` | cursor start (disabled) | `0x06` |
| R11 | `0x00` | cursor end (disabled) | `0x07` |
| R12/R13 | `0x00` | start address 0 | `0x00` |

`R9=1` is normal for CGA modes 4/5/6; the only nonstandard timing value is
**`R2=0x2C`** (PCjr), plus the disabled cursor registers.

### Framebuffer layout

From the pixel routine at `CS:0x05CF` (`file 0x04CF`):

```
addr = (y & ~1) * 40 + (y & 1) * 0x2000 + (x >> 2)
```

- 320×200, 2 bits/pixel, 80 bytes/scanline, 16 KB at `B800:0000`.
- Even scanlines in bank 0 (`B800:0000`), odd scanlines in bank 1
  (`B800:2000`).
- Each byte holds four pixels, leftmost in bits 7–6, then 5–4, 3–2, 1–0.
- `R9=1` matches this: one character row = one even/odd scanline pair.

The title screen is an RLE-compressed 16 KB bitmap at `DS:0x0DAC`, blitted
sequentially into the framebuffer by `CS:0x17EB` (`0xFE count value` escape;
`count` copies, with 0 meaning 256).
`DS:0x0DAC` is therefore already in the CGA interleaved layout.

### Palette/background

`3D9` is cached at `DS:0x03B5` and written by:

- `CS:0x04E7` (`file 0x03E7`): bit 5 — palette select (`0` = green/red/brown,
  `1` = cyan/magenta/white).
- `CS:0x0520` (`file 0x0420`): bits 0–3 — background colour.
- `CS:0x054B`/`0x0556`: bit 4 — intensity.

Writes are preceded by the retrace wait at `CS:0x050B` when the retrace flag
(`DS:0x03B3`) is set.

### Runtime horizontal shift

The keyboard handler at `CS:0x1128`/`0x1155` (`file 0x1028`/`0x1055`)
rewrites **CRTC R2** via `3D4/3D5`:

```
; DS:0x0673 initially contains 0x2C (the table's R2 value)
inc byte DS:0x0673          ; clamp 0x2A..0x2E
out 3D4h, 2
out 3D5h, [DS:0x0673]
```

This shifts the display horizontally by one character (8 pixels). On real CGA
hardware it changes only the H-sync timing; it is not a framebuffer scroll.

### Startup message

`DS:0x0263` is the `$`-terminated "Please turn on display connected to your
Color Graphics Display Adapter" string; `DS:0x0322` is "This program requires
the Color/Graphics Display Adapter". They are printed through INT 10h
AH=0Eh by `CS:0x0456`, **after** the graphics CRTC has been installed. The
printable substrings reported by `strings` at file offsets `0x9005`,
`0x9042`, and `0x90C4` are inside these two messages (DS `0x0295`, `0x02D2`,
`0x0354`).

## Why DOSBox fails

DOSBox's `src/hardware/vga_other.cpp:write_cga()` handles `3D8h/3D9h` **only
when `machine==MCH_CGA`**. It maps `0x02`/`0x0A` to `M_TANDY4` (CGA
320×200×4), calls `write_color_select()` and `VGA_SetBlinking()`, and leaves
the CRTC/renderer setup to `VGA_SetMode()`.

`VGA_SetupDrawing()` for `M_TANDY4` uses the `vga.other` timing values
(`hdend`, `vdend`, `max_scanline`), `doubleheight=true`,
`doublewidth=(mode_control & 1) == 0`, a fixed 80-byte source stride, and the
CGA line handler's `line_mask`/`line_shift`/`addr_mask` for the interleaved
banks. It does not use CRTC `R2` for the visible width.

Two failure modes follow:

1. **Non-CGA machine (DOSBox default, EGA/VGA/Hercules).** The `3D8h/3D9h`
   writes are not handled by `write_cga()` at all, so the VGA stays in VGA
   mode while the game writes CGA CRTC values and a CGA framebuffer. If the
   VGA is still in its text mode, the 2bpp title framebuffer is rendered as
   80×25 characters/attributes; that is the repeated-text/green-box image in
   `dosbox_digger.png`. This is the classic "direct 3D8" failure
   documented on VOGONS.
2. **CGA machine.** `write_cga()` selects `M_TANDY4`, but the game writes
   CRTC R0–R13 **after** the mode switch. If DOSBox's CRTC write path does
   not update `vga.other.hdend/vdend/max_scanline`, the CGA line state, and
   the renderer dimensions, the renderer can use stale timing/stride. The
   nonstandard `R2=0x2C` and the runtime `R2` rewrites are H-sync only and
   must not change the visible width or wrap the framebuffer; if they do, the
   display repeats/tiles. The BIOS teletype/scroll used for the startup
   message also depends on the current CRTC geometry.

The game's own workaround options are documented: call INT 10h mode 4
instead of `OUT 3D8h` (VOGONS patch at file `0x38F`:
`BA D8 03 B0 02 EE` → `B8 04 00 CD 10 90`), and force palette 0/black
background (archive "palette fix", which on that build replaces the custom
CRTC setup at `0x32C` with INT 10h mode 4 + palette calls).

## Working reference

The Internet Archive item **Digger (working CGA)** is the unpatched
`digger83/digdug.com` with:

```
[dosbox]
machine=cga
[cpu]
cycles=fixed 400
```

Its screenshots (`screenshot_00.png` title, `00_coverscreenshot.png`
gameplay) show the correct 320×200×4 display. The left side of
`dosbox_digger.png` is this archive build (DOSBox window title
"Program: DIGDUG"). The archive's `digdug.com` and this repository's
`digger.com` have the **same CGA code and CRTC table at the same file
offsets** (the binaries differ elsewhere; SHA-256 differs).

## Fix directions for DOSBox

1. **First verify the configuration**: `machine=cga` and a slow 8088-like
   cycle setting (the archive uses `cycles=fixed 400`). At `cycles=3000` the
   game's timing-sensitive direct CRTC/3D8 writes may not land where the
   emulation expects.
2. **For non-CGA machines**: make the VGA/EGA 3D8 path detect the CGA
   mode-control writes and perform a CGA-compatible mode-4 switch (the
   equivalent of BIOS INT 10h mode 4), or emulate the CGA CRTC overlay.
   This is the code-level version of the VOGONS patch.
3. **For `machine=cga`**: audit `write_crtc()` / `VGA_SetupDrawing()` /
   `VGA_StartResize()` and the CGA line state for direct R0–R13 writes while
   video is off. Ensure the fixed 80-byte stride and interleave masks are
   derived from the new timing, and treat R2 as H-sync only. Check the BIOS
   teletype/scroll path used for the startup message under the custom CRTC.
4. Compare DOSBox-X's `vga_other.cpp` / `vga_draw.cpp` and MAME's CGA device
   for the CRTC R2/R9 semantics.

## Sources

- [VOGONS: Digger and Hercules Graphics, using CGA emulation](https://www.vogons.org/viewtopic.php?t=33424)
- [Internet Archive: Digger (working CGA)](https://archive.org/details/digger_cga)
- [Internet Archive: Digger (with palette fix)](https://archive.org/details/digger_with_palette_fix)
- [reenigne: 8088/CGA register values](https://github.com/reenigne/reenigne/blob/master/8088/cga/register_values.txt)
- [DOSBox vga_other.cpp (blackberry mirror)](https://github.com/blackberry/DosBox/blob/master/src/hardware/vga_other.cpp)
- [DOSBox-X vga_other.cpp source](https://dosbox-x.com/doxygen/html/vga__other_8cpp_source.html)
- [int10h.org: Astro-Dodge's Dirty Video Tricks](https://int10h.org/blog/2023/10/astro-dodge-cga-dirty-video-tricks/)

## Resolution (2026-10-24)

The archive item [`digger_cga`](https://archive.org/details/digger_cga) contains
`digger83b.zip`, whose DOSBox configuration runs the game correctly on current
DOSBox.  The `digger.com` in that archive is byte-identical to the repo's
`digger.com` apart from the embedded high-score section, which is expected.
No DOSBox code change is needed for this build: the working configuration is
`machine=cga` (with the archive's cycle setting) rather than the default
EGA/VGA machine.

## Data formats

### Title bitmap — `DS:0x0DAC`

The title image is RLE-compressed into the 16 KB CGA framebuffer:

```
while (out < 0x4000)
    if (byte == 0xFE) { count = next; value = next; repeat value count times; }
    else                out[di++] = byte;
```

The blitter is `CS:0x17EB`; it writes directly to `B800:0000`.  The stream is
2523 bytes and its tail (from `DS:0x16E4`) overlaps the first 163 bytes of the
sprite bank; those bytes only affect the final two scanlines.

### Packing

There is no single large bitmap for the sprites or the font.  The font is a
contiguous array of 0x27-byte glyph records at `DS:0x06B6`; sprite frames are
packed data/mask bank pairs from `DS:0x16E4` to `DS:0x3B23`, with 1-byte
padding and scratch (saved-background) blocks interleaved.  The background is
eight 5x4 tiles rather than a bitmap.  Only the title screen is a full-screen
bitmap, and it is RLE-compressed.

### Sprite frames — DS source pairs

Each moving sprite has two DS source blocks passed to `setspr`/`setspr2`:
a colour/data block and a transparency-mask block.  A frame is `height` rows
of `width` bytes (2bpp, 4 pixels per byte, leftmost in bits 7–6); drawing
computes `(screen & mask) | data` per byte.  The sizes below are in bytes;
multiply the width by four for pixels.  Frame sizes:

| Sprite | Size | Example sources |
|---|---|---|
| bag/gold, monster, digger | 4x15 | `0x18D6/0x1913`, `0x1DD0/0x1E0D` |
| monster death (left) | 4x14 | `0x24EE/0x2527` |
| emerald | 4x10 | `0x16E4/0x170D` |
| life icon | 4x12 | `0x17B0/0x17E1` |
| fireball | 2x7 | `0x2A2A/0x2A39` |
| wall blob | 2x18 | `0x2706/0x272B` |
| wall blob | 6x6 | `0x279A/0x27BF` |
| furry blob | 6x8 | `0x282E/0x285F` |
| background tile | 5x4 | `0x28DA/0x28EF` … `0x2A00/0x2A15` |

### Title font — `DS:0x06B6`

Sixty-four glyph pointers for characters `0x20..0x5F` (table at
`CS:0x1523`).  Each glyph is `[width in bytes][height][2bpp rows]`, stored in
`0x27`-byte slots; only space, `.`, `0-9`, `A-Z` and `_` exist.  Text cells
advance 12 pixels.

### Background

`drawbackg` tiles a 5x4 source pair over the 320x186 area for `y=14..199`;
the eight tile sets are selected by `levplan()`.

### Extracted files

`tools/extract_data.py` writes every table and graphic here to `data/`
(raw segment images, tables, all 76 sprite frames, the 39 font glyphs, the
title RLE/framebuffer and PNGs); see `data/README.md`.  The PNGs use
palette 0, low intensity (black/green/red/brown) and are nearest-neighbour
4x upscales of the native 2bpp images.  `data/` is generated and ignored
by git; only the tool is committed.
