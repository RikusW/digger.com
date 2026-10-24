# Extracted data

Generated from `digger.com` by `tools/extract_data.py`.  Do not edit
these files by hand; re-run the tool instead.  This directory is ignored by
git (`data/` in the root `.gitignore`); only the tool is committed.

## Layout

```
raw/digger_cs.bin      code segment image (file 0x0000..0x8D70)
raw/digger_ds.bin      data segment image (file 0x8D70..0xE100)
tables/                levels, music, RNG, ctype, scores, strings, orders,
                       keyboard scan-code table, copyright string
font/                  glyphs.json, U+XXXX.bin/.png (39 glyphs),
                       dispatch.json (handler addresses), font_raw.bin
sprites/               every sprite frame: *_data.bin, *_mask.bin, *.png;
                       sprite_banks.bin (raw DS:0x16E4..0x3B00)
title/                 title.rle.bin, title.bin (16 KB framebuffer), title.png
manifest.json          every table/sprite with segment, offset, size
```

`DS` offsets map to file offset `DS + 0x8D70`; `CS` offsets map to
`CS - 0x100`.

## Formats

- **Pixels**: 2 bits per pixel, 4 pixels per byte, leftmost in bits 7-6.
  The game selects CGA palette 0, low intensity: 0 black, 1 green, 2 red,
  3 brown (`00AA00`, `AA0000`, `AA5500`).  PNGs are nearest-neighbour 4x
  upscales (one source pixel is a 4x4 block); the `.bin` banks stay native.
  Widths in `manifest.json`/`sprites.json` are in **bytes**; a sprite is four
  times that many pixels wide (e.g. 4 bytes = 16 px), and the PNG is another
  4x larger (64 px).
- **Packing**: there is no single large sprite or font bitmap.  The font is a
  contiguous array of 0x27-byte records at `DS:0x06B6`; sprites are packed
  data/mask bank pairs from `DS:0x16E4` to `DS:0x3B24`, with scratch
  (saved-background) blocks interleaved among them.  Only the title screen is
  a full-screen bitmap, and it is RLE-compressed.
- **Sprite frames**: two banks of `width*height` bytes.  The *data* bank holds
  the colour; the *mask* bank is the keep mask used by
  `screen = (screen & mask) | data`.  A pixel with mask 3 is transparent;
  the PNGs use alpha `(3 - mask) * 85`.
- **Title bitmap**: `title.rle.bin` is the compressed stream at `DS:0x0DAC`.
  `0xFE count value` repeats `value` `count` times (a stored count of 0 means
  256); literal bytes are copied.  The stream is 2523 bytes and decodes to
  exactly 16 KB, the interleaved CGA framebuffer (`title.bin`):
  `addr = (y&~1)*40 + (y&1)*0x2000 + x/4`.  Its last ~163 source bytes come
  from the start of the sprite bank; they only produce the final two
  scanlines, so the overlap is harmless.
- **Font**: each glyph record is `[width in bytes][height][2bpp rows]` and
  lives in the data segment; the 64-entry dispatch table is at `CS:0x1523`.
  Text cells advance 12 pixels.
- **Levels**: eight 150-byte arrays (10 rows of 15 characters), also rendered
  in `tables/levels.txt`.
- **Music**: word arrays of `[note, duration]` pairs, terminated by the
  `0x7D64` sentinel (see `C/sound.c`).

## Regenerate

```
python3 tools/extract_data.py
python3 tools/compare_tables.py   # verifies the reconstructed C tables
```
