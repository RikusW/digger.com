#!/usr/bin/env python3
"""Extract every known data table and graphic from digger.com.

Outputs live in data/:

  raw/        the complete code and data segment images
  tables/     levels, music, RNG, ctype, scores, messages, strings, orders
  font/       the 64-glyph title font (JSON + PNGs)
  sprites/    every sprite frame (data/mask banks, PNG, manifest)
  title/      the RLE title image, the decoded 16 KB framebuffer and PNG

The layouts (2bpp pixels, data/mask banks, RLE, glyph records) are documented
in data/README.md and CGA.md.  Pure Python 3, no dependencies.

Usage: python3 tools/extract_data.py
"""

import json
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "digger.com"
OUT = ROOT / "data"

DS_BASE = 0x8D70
DS_SIZE = 0x5390
CS_SIZE = 0x8D70

# CGA palette 0, low intensity: black, green, red, brown.
# The game selects palette 0 in gpal(0) after cga_init's 3D9=0x20.
PALETTE = [(0, 0, 0), (0, 170, 0), (170, 0, 0), (170, 85, 0)]
SCALE = 4                      # PNGs are nearest-neighbour 4x upscales


def ds(off):
    return DS_BASE + off


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data if isinstance(data, (bytes, bytearray)) else bytes(data))


def write_json(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n")


# ---------------------------------------------------------------- PNG output
def png_chunk(tag, data):
    return (struct.pack(">I", len(data)) + tag + data +
            struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))


def write_png(path, width, height, rgba, scale=SCALE):
    if scale != 1:
        big = bytearray()
        for y in range(height):
            row = rgba[y * width * 4:(y + 1) * width * 4]
            wide = bytearray()
            for i in range(0, len(row), 4):
                px = row[i:i + 4]
                wide += px * scale
            for _ in range(scale):
                big += wide
        rgba = big
        width *= scale
        height *= scale
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        raw += rgba[y * stride:(y + 1) * stride]
    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += png_chunk(b"IEND", b"")
    write(path, png)


def cga_pixel(fb, x, y):
    """2-bit pixel from the interleaved 16 KB CGA framebuffer."""
    addr = (y & ~1) * 40 + (y & 1) * 0x2000 + (x >> 2)
    return (fb[addr] >> ((3 - (x & 3)) * 2)) & 3


def fb_png(fb, path, width=320, height=200):
    rgba = bytearray()
    for y in range(height):
        for x in range(width):
            r, g, b = PALETTE[cga_pixel(fb, x, y)]
            rgba += bytes((r, g, b, 255))
    write_png(path, width, height, rgba)


def bank_pixel(bank, stride, x, y):
    return (bank[y * stride + (x >> 2)] >> ((3 - (x & 3)) * 2)) & 3


def sprite_png(data, mask, path, width_bytes, height):
    width = width_bytes * 4          # 2bpp: four pixels per byte
    rgba = bytearray()
    for y in range(height):
        for x in range(width):
            v = bank_pixel(data, width_bytes, x, y)
            keep = bank_pixel(mask, width_bytes, x, y)
            r, g, b = PALETTE[v]
            alpha = (3 - keep) * 85
            rgba += bytes((r, g, b, alpha))
    write_png(path, width, height, rgba)


# ------------------------------------------------------------------- tables
def decode_rle(src, limit=0x4000):
    out = bytearray()
    i = 0
    while len(out) < limit:
        b = src[i]
        i += 1
        if b == 0xFE:
            count = src[i]
            value = src[i + 1]
            i += 2
            # The ISR writes once then loops `dec count; jne`: count bytes
            # for 1..255, and 256 when the stored count is zero.
            out += bytes((value,)) * (count if count else 256)
        else:
            out.append(b)
    return bytes(out[:limit]), i


def nul_strings(blob, start, end):
    rows = []
    i = start
    while i < end:
        if blob[i] == 0:
            i += 1
            continue
        j = i
        while j < end and blob[j] != 0:
            j += 1
        rows.append({"offset": i, "text": blob[i:j].decode("latin-1")})
        i = j
    return rows


def main():
    blob = BIN.read_bytes()
    if len(blob) != DS_BASE + DS_SIZE:
        raise SystemExit("unexpected digger.com size: %d" % len(blob))
    dsimg = blob[DS_BASE:DS_BASE + DS_SIZE]
    csimg = blob[:CS_SIZE]

    write(OUT / "raw" / "digger_cs.bin", csimg)
    write(OUT / "raw" / "digger_ds.bin", dsimg)

    manifest = []

    def entry(name, seg, off, size, path, desc):
        manifest.append({
            "name": name, "segment": seg, "offset": off,
            "file_offset": (DS_BASE if seg == "DS" else -0x100) + off,
            "size": size, "path": path, "description": desc,
        })

    # ---- simple binary/word tables
    tables = [
        ("cga_crtc", 0x0255, 14, "CGA CRTC R0..R13 (R2=0x2C PCjr H-sync)"),
        ("rng_table", 0x0CA7, 256, "256-byte RNG permutation table"),
        ("draw_order", 0x0627, 32, "default sprite draw order (16 words)"),
        ("title_draw_order", 0x4FBE, 32, "title-screen sprite draw order (16 words)"),
        ("music_t1", 0x45E4, 172, "bonus jingle T1 (86 words)"),
        ("score_default", 0x4690, 112, "embedded default score block"),
        ("music_t2", 0x4700, 358, "bonus jingle T2 (179 words)"),
        ("music_background", 0x4866, 582, "background jingle (291 words)"),
        ("music_dirge", 0x4AAC, 98, "death jingle (49 words)"),
        ("levels", 0x4B0E, 1200, "eight 150-byte level arrays"),
        ("ctype", 0x509A, 257, "C character-class table"),
    ]
    for name, off, size, desc in tables:
        data = dsimg[off:off + size]
        write(OUT / "tables" / (name + ".bin"), data)
        entry(name, "DS", off, size, "tables/%s.bin" % name, desc)
        if size % 2 == 0 and name != "rng_table" and name != "cga_crtc" and name != "ctype":
            words = list(struct.unpack("<%dH" % (size // 2), data))
            write_json(OUT / "tables" / (name + ".json"), words)
    write_json(OUT / "tables" / "rng_table.json", list(dsimg[0x0CA7:0x0CA7 + 256]))
    write_json(OUT / "tables" / "cga_crtc.json", list(dsimg[0x0255:0x0255 + 14]))

    levels = dsimg[0x4B0E:0x4B0E + 1200]
    lines = []
    for lev in range(8):
        lines.append("level %d (DS:0x%04X)" % (lev + 1, 0x4B0E + lev * 150))
        for row in range(10):
            lines.append(levels[lev * 150 + row * 15: lev * 150 + row * 15 + 15]
                         .decode("latin-1"))
        lines.append("")
    (OUT / "tables" / "levels.txt").write_text("\n".join(lines))

    # ---- keyboard scan-code table (DS:0x0676, 21 entries of 3 bytes)
    kbd = []
    for i in range(0x15):
        off = 0x0676 + i * 3
        scan = dsimg[off]
        target = struct.unpack_from("<H", dsimg, off + 1)[0]
        kbd.append({"scan": scan, "target": target})
    write_json(OUT / "tables" / "keyboard_table.json", kbd)
    write(OUT / "tables" / "keyboard_table.bin", dsimg[0x0676:0x0676 + 63])
    entry("keyboard_table", "DS", 0x0676, 63, "tables/keyboard_table.bin",
          "21 scan-code entries: scan byte + CS handler address")

    # ---- raw regions for completeness
    write(OUT / "font" / "font_raw.bin", dsimg[0x06B6:0x0CA7])
    entry("font_raw", "DS", 0x06B6, 0x0CA7 - 0x06B6, "font/font_raw.bin",
          "raw glyph-data region (39 x 0x27-byte records)")

    # ---- copyright string in the code segment (CS:0x8E40)
    copy_off = 0x8E40 - 0x100
    copy_end = csimg.index(b"\x00", copy_off)
    copyright_text = csimg[copy_off:copy_end].decode("latin-1")
    write_json(OUT / "tables" / "copyright.json", {
        "offset": 0x8E40, "file_offset": copy_off, "text": copyright_text})
    entry("copyright", "CS", 0x8E40, copy_end - copy_off + 1,
          "tables/copyright.json", "copyright string")

    # ---- messages and strings
    msgs = []
    for off in (0x0263, 0x0322):
        end = dsimg.index(b"$", off)
        text = dsimg[off:end].decode("latin-1")
        msgs.append({"offset": off, "text": text})
        entry("message_%04X" % off, "DS", off, end - off + 1,
              "tables/messages.txt", "CGA startup message")
    write_json(OUT / "tables" / "messages.json", msgs)
    (OUT / "tables" / "messages.txt").write_text(
        "\n".join("DS:0x%04X %s" % (m["offset"], m["text"]) for m in msgs) + "\n")

    strings = []
    strings += nul_strings(dsimg, 0x4166, 0x41E0)
    strings += nul_strings(dsimg, 0x5018, 0x509A)
    write_json(OUT / "tables" / "strings.json", strings)
    (OUT / "tables" / "strings.txt").write_text(
        "\n".join("DS:0x%04X %r" % (s["offset"], s["text"]) for s in strings) + "\n")

    # ---- title bitmap
    rle, used = decode_rle(dsimg[0x0DAC:])
    write(OUT / "title" / "title.rle.bin", dsimg[0x0DAC:0x0DAC + used])
    write(OUT / "title" / "title.bin", rle)
    fb_png(rle, OUT / "title" / "title.png")
    entry("title_rle", "DS", 0x0DAC, used, "title/title.rle.bin",
          "RLE stream: 0xFE count value, decodes to 16 KB at B800")
    entry("title_framebuffer", "DS", 0x0DAC, 0x4000, "title/title.bin",
          "decoded 16 KB CGA framebuffer")

    # ---- font
    glyph_ptr = {}
    table_cs = 0x1523 - 0x100   # file index of the CS:0x1523 table
    for idx in range(64):
        target = struct.unpack_from("<H", csimg, table_cs + idx * 2)[0]
        # A glyph handler is `push color; mov ax,imm16; push ax; push x;
        # push y; call drawglyph`.  Accept the imm only when the handler
        # really calls CS:0x08E5 (file 0x07E5).
        ptr = None
        start = target - 0x100
        for k in range(start, min(start + 48, len(csimg) - 6)):
            if csimg[k] != 0xB8:
                continue
            imm = struct.unpack_from("<H", csimg, k + 1)[0]
            for j in range(k + 3, min(k + 20, len(csimg) - 3)):
                if csimg[j] == 0xE8:
                    rel = struct.unpack_from("<h", csimg, j + 1)[0]
                    if j + 3 + rel == 0x07E5:
                        ptr = imm
                        break
            if ptr is not None:
                break
        if ptr is not None:
            glyph_ptr[0x20 + idx] = ptr
    glyphs = []
    for code in sorted(glyph_ptr):
        p = glyph_ptr[code]
        w = dsimg[p]
        h = dsimg[p + 1]
        data = dsimg[p + 2:p + 2 + w * h]
        name = "U+%04X" % code
        write(OUT / "font" / (name + ".bin"), bytes((w, h)) + data)
        rgba = bytearray()
        for y in range(h):
            for x in range(w * 4):
                v = bank_pixel(data, w, x, y)
                r, g, b = PALETTE[v]
                rgba += bytes((r, g, b, 255 if v else 0))
        write_png(OUT / "font" / (name + ".png"), w * 4, h, rgba)
        glyphs.append({"char": chr(code), "code": code, "offset": p,
                       "width": w, "height": h,
                       "rows": [data[y * w:(y + 1) * w].hex() for y in range(h)]})
    write_json(OUT / "font" / "glyphs.json", glyphs)
    dispatch = []
    for idx in range(64):
        target = struct.unpack_from("<H", csimg, 0x1523 - 0x100 + idx * 2)[0]
        dispatch.append({"char": chr(0x20 + idx), "target": target})
    write_json(OUT / "font" / "dispatch.json", dispatch)
    write(OUT / "font" / "dispatch.bin",
          csimg[0x1523 - 0x100:0x1523 - 0x100 + 128])
    entry("font_dispatch", "CS", 0x1523, 128, "font/dispatch.bin",
          "64 glyph-handler addresses (chars 0x20..0x5F)")

    # ---- sprites
    walk = {
        "right": [[(0x2C58, 0x2C95), (0x2DC6, 0x2E03)],
                   [(0x2CD2, 0x2D0F), (0x2E40, 0x2E7D)],
                   [(0x2D4C, 0x2D89), (0x2EBA, 0x2EF7)]],
        "up":    [[(0x3210, 0x324D), (0x33BA, 0x33F7)],
                   [(0x328A, 0x32C7), (0x3434, 0x3471)],
                   [(0x3340, 0x337D), (0x34EA, 0x3527)]],
        "left":  [[(0x2F34, 0x2F71), (0x30A2, 0x30DF)],
                   [(0x2FAE, 0x2FEB), (0x311C, 0x3159)],
                   [(0x3028, 0x3065), (0x3196, 0x31D3)]],
        "down":  [[(0x3564, 0x35A1), (0x36D2, 0x370F)],
                   [(0x35DE, 0x361B), (0x374C, 0x3789)],
                   [(0x3658, 0x3695), (0x37C6, 0x3803)]],
    }
    frames = [
        ("digger_initial_buffer", 0x2BA2, 0x2BDF, 4, 15),
        ("emerald", 0x16E4, 0x170D, 4, 10),
        ("emerald_erase", 0x175E, 0x1787, 4, 10),
        ("life_p1", 0x17B0, 0x17E1, 4, 12),
        ("life_p2", 0x1812, 0x1843, 4, 12),
        ("life_empty", 0x1874, 0x18A5, 4, 12),
        ("monster_die_hobbin", 0x1F3E, 0x1F7B, 4, 15),
        ("monster_die_right", 0x2216, 0x2253, 4, 15),
        ("monster_die_left", 0x24EE, 0x2527, 4, 14),
        ("bonus", 0x2AEC, 0x2B29, 4, 15),
        ("fire_0", 0x2A2A, 0x2A39, 2, 7),
        ("fire_1", 0x2A56, 0x2A65, 2, 7),
        ("fire_2", 0x2A74, 0x2A83, 2, 7),
        ("explosion_0", 0x2A92, 0x2AA1, 2, 7),
        ("explosion_1", 0x2AB0, 0x2ABF, 2, 7),
        ("explosion_2", 0x2ACE, 0x2ADD, 2, 7),
        ("blob_right", 0x2706, 0x272B, 2, 18),
        ("blob_left", 0x2750, 0x2775, 2, 18),
        ("blob_top", 0x279A, 0x27BF, 6, 6),
        ("blob_bottom", 0x27E4, 0x2809, 6, 6),
        ("blob_furry", 0x282E, 0x285F, 6, 8),
        ("blob_square", 0x2890, 0x28B5, 6, 6),
    ]
    for i, (a, b) in enumerate([
        (0x18D6, 0x1913), (0x198C, 0x19C9), (0x1A06, 0x1A43),
        (0x1A80, 0x1ABD), (0x1AFA, 0x1B37), (0x1B74, 0x1BB1),
        (0x1BEE, 0x1C2B)]):
        frames.append(("gold_%d" % i, a, b, 4, 15))
    for leg, pairs in enumerate([(0x1DD0, 0x1E0D), (0x1E4A, 0x1E87),
                                 (0x1EC4, 0x1F01)]):
        frames.append(("monster_hobbin_%d" % leg, pairs[0], pairs[1], 4, 15))
    for leg, pairs in enumerate([(0x1FF4, 0x2031), (0x20AA, 0x20E7),
                                 (0x2160, 0x219D)]):
        frames.append(("monster_right_%d" % leg, pairs[0], pairs[1], 4, 15))
    for leg, pairs in enumerate([(0x22CC, 0x2309), (0x2382, 0x23BF),
                                 (0x2438, 0x2475)]):
        frames.append(("monster_left_%d" % leg, pairs[0], pairs[1], 4, 15))
    for d, legs in walk.items():
        for leg, pairs in enumerate(legs):
            frames.append(("digger_%s_leg%d_walk" % (d, leg), pairs[0][0], pairs[0][1], 4, 15))
            frames.append(("digger_%s_leg%d_fire" % (d, leg), pairs[1][0], pairs[1][1], 4, 15))
    for i, (a, b) in enumerate([(0x3840, 0x387D), (0x38BA, 0x38F7),
                                (0x3934, 0x3971), (0x39AE, 0x39EB),
                                (0x3A28, 0x3A65), (0x3AA2, 0x3AE7)]):
        frames.append(("digger_die_%d" % i, a, b, 4, 15))
    for i, (a, b) in enumerate([(0x28DA, 0x28EF), (0x2904, 0x2919),
                                (0x292E, 0x2943), (0x2958, 0x296D),
                                (0x2982, 0x2997), (0x29AC, 0x29C1),
                                (0x29D6, 0x29EB), (0x2A00, 0x2A15)]):
        frames.append(("tile_%d" % i, a, b, 5, 4))

    sprite_manifest = []
    for name, src0, src1, w, h in frames:
        n = w * h
        data = dsimg[src0:src0 + n]
        mask = dsimg[src1:src1 + n]
        if len(data) != n or len(mask) != n:
            raise SystemExit("sprite out of range: %s" % name)
        write(OUT / "sprites" / (name + "_data.bin"), data)
        write(OUT / "sprites" / (name + "_mask.bin"), mask)
        sprite_png(data, mask, OUT / "sprites" / (name + ".png"), w, h)
        sprite_manifest.append({"name": name, "data_offset": src0,
                                "mask_offset": src1, "width_bytes": w,
                                "width_pixels": w * 4, "height": h,
                                "size_bytes_per_bank": n})
        entry("sprite_" + name, "DS", src0, n, "sprites/%s_data.bin" % name,
              "sprite data bank (mask at DS:0x%04X)" % src1)
    bank_end = max(src1 + w * h for _, src0, src1, w, h in frames)
    write(OUT / "sprites" / "sprite_banks.bin", dsimg[0x16E4:bank_end])
    entry("sprite_banks", "DS", 0x16E4, bank_end - 0x16E4,
          "sprites/sprite_banks.bin", "raw packed sprite bank region")
    write_json(OUT / "sprites" / "sprites.json", sprite_manifest)

    # ---- README
    readme = """# Extracted data

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
"""
    (OUT / "README.md").write_text(readme)

    # ---- manifest
    write_json(OUT / "manifest.json", {
        "source": "digger.com",
        "ds_base_file_offset": DS_BASE,
        "code_segment_bytes": CS_SIZE,
        "data_segment_bytes": DS_SIZE,
        "palette": PALETTE,
        "tables": manifest,
        "sprites": sprite_manifest,
        "glyphs": glyphs,
    })
    print("wrote %d table entries, %d sprites, %d glyphs" %
          (len(manifest), len(sprite_manifest), len(glyphs)))


if __name__ == "__main__":
    main()
