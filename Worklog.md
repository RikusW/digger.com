# Worklog

## Add speaker simulation filter to SDL sound output

### User

This is a 1983 game that was reverse engineered to C. It used a PWM sound system but it seems like it was wired directly to audio output and does not sound like the original, there is a lot of noise. A filter simulating a speaker should improved this, add it in sdl_snd.c, this is probably the best place for it to go ?

### Assistant

I'll look at the sound code and how the PWM output is generated.

### Assistant

Now I understand the signal path: `getsample()` in `newsnd.c` emits the raw PC-speaker position (0 = off, 255 = on) averaged per output sample, and `sdl_snd.c:fill_audio` feeds it straight to SDL. That's an un-filtered square wave with DC offset — exactly what a real speaker can't reproduce. I'll add the speaker model at the SDL output stage.

### Assistant

Builds. Now let's design and implement.

### Assistant

Build is clean. Now let me verify the actual filter behavior end-to-end by running the real `sdl_snd.c` code against a generated PC-speaker waveform using SDL's disk audio driver.

### Assistant

The filter math works, but analysis exposed a subtlety: a band-limited square wave peaks at up to ~4/π·(gain) — removing harmonics raises the peak, not lowers it — so the 0.90 gain clipped steadily. Let me set proper headroom.

### Assistant

The 150 Hz test clips because the 2nd-order high-pass sits too close to the game's lowest notes (147 Hz) and droops/rings. A real speaker's AC coupling is first-order anyway, so I'll switch the high-pass to a gentler first-order section and retune.

### Assistant

Tuning analysis shows the 2nd-order high-pass is the wrong model: it differentiates low bass notes (the game sweeps down to ~40 Hz) and forces clipping. A real speaker's AC coupling is first-order, and the game's lowest notes need it to be very low. Let me fix that.

### Assistant

Let me do one final integration test that drives the actual sound engine (`sound.c` + `newsnd.c` + `sdl_snd.c`) rather than a synthetic waveform.

### Assistant

Yes — `sdl_snd.c` is the right place. It's the only SDL-specific stage that sees the raw speaker samples, and it already knows the sample rate, so it can configure the filter per output rate.

## What was wrong

`getsample()` (`newsnd.c`) returns the PC speaker's instantaneous position as 0 (`off`) or 255 (`on`) averaged over one output sample. `fill_audio()` was feeding that straight to SDL, so the sound card played:

- a hard square wave with all its harmonics (the harsh "noise"), and
- a large DC offset — "speaker off" is `0`, which is a negative DC level for 8-bit audio.

A real PC speaker can't reproduce DC and its cone rolls off the high harmonics. That physical stage was missing entirely.

## The filter (`sdl_snd.c:17-130`)

I added a small speaker model in `fill_audio()`:

1. **1st-order high-pass @ 5 Hz** — removes the DC, like the speaker's suspension. It's deliberately very low: the game's dying/explosion sweeps go down to ~40 Hz, and any higher corner either guts them or differentiates them into clipping (a 2nd-order 100 Hz version I tried did exactly that).
2. **2nd-order Butterworth low-pass @ 5 kHz** (RBJ coefficients, clamped to 40% of the sample rate) — the cone-inertia rolloff that removes the buzzy harmonics. 12 dB/oct with no ringing.
3. **0.70 gain + clamp** — a band-limited square peaks near 4/π, not 1.0, so this headroom prevents the filter itself from clipping.

Each biquad uses transposed direct-form II in `double`; state is reset in `killsounddevice()`.

## Tuning

The three knobs are at `sdl_snd.c:36-42`:

```c
#define SPEAKER_HP_HZ 5.0
#define SPEAKER_LP_HZ 5000.0
#define SPEAKER_GAIN  0.70
```

Lower `SPEAKER_LP_HZ` for a duller/softer sound, raise it for more bite. Raising `SPEAKER_GAIN` increases loudness but will start clipping the deepest sweeps and sound onsets.

## Verification

- Clean build with no new warnings; headless run with audio enabled (`Device=1`) works.
- Captured the **real sound engine** through the real `sdl_snd.c` using SDL's disk audio driver at 22050 Hz, filtered vs unfiltered:

| band | unfiltered | filtered |
|---|---|---|
| 1–3 kHz | −0.5 dB | −0.2 dB |
| 5–8 kHz | −11.6 dB | −19.1 dB |
| 8–11 kHz | −18.2 dB | −44.9 dB |

- Steady 1 kHz and 150 Hz test tones come through at the correct pitch with **no clipping** (min/max ≈ 21/254), and silence maps to 128 instead of 0.

One incidental change: `-lm` was added to `Makefile.sdl` (Linux/FreeBSD) and `Makefile.fbsd`, since the filter uses `sin`/`cos`/`tan` at init time. I did not commit anything.

### User

commit it

### Assistant

Committed as `da079ad` on `master`:

```
Add speaker simulation filter to SDL sound output
 3 files changed, 129 insertions(+), 5 deletions(-)
```

Only `sdl_snd.c`, `Makefile.sdl`, and `Makefile.fbsd` went into the commit.

Note: the working tree now shows untracked build artifacts (`.o` files, `digger`) plus `Agents.md`/`.Agents.md.swp` that appeared after my earlier `make clean`. I left them alone and did not include them in the commit.

### User

check Agents.md and update the commit

---

*This commit was amended after the exchange above to include this worklog, as required by `Agents.md`. `da079ad` is the pre-amend hash mentioned in the reply above.*

## Fix music aliasing by oversampling the speaker emulation

### User

The game effects sound much like the original but the music and end of level tune still sounds a bit distorted

### Assistant

The music path uses extremely narrow PWM pulses (often under one output sample). That makes aliasing a prime suspect — unlike the effects' square waves, a narrow pulse has near-flat energy right up to Nyquist. Let me test the same music at 44.1 kHz to see if the inter-harmonic noise disappears.

### Assistant

The PSD comparison confirms it: at 22.05 kHz the native capture has ~5–7 dB more energy above 3 kHz than the same music emulated at 44.1 kHz — that's aliasing from the narrow PWM pulses folding back into the audible band. Let me check your actual config before deciding how to fix it.

### Assistant

The PSD difference is clear: 22.05 kHz has +5–7 dB of high-frequency hash versus the 44.1 kHz reference — aliasing from the sub-sample PWM pulses. Effects use square waves (1/n harmonics) so they're much less affected, which matches exactly what you hear. The fix is to oversample the emulation internally and decimate, so it works even if you launch with `/A:...,22050,...`.

### Assistant

The oversampled emulation now matches the 44.1 kHz reference within 0.6 dB across all bands, versus +3 to +7 dB of aliasing before. Let me smoke-test the game with both `/A` rates and rebuild cleanly.

### Assistant

The `/A` run now crashes with SIGFPE. Let me isolate whether it's the new code or the pre-existing `/A` parsing.

### Assistant

The SIGFPE exposed a pre-existing bug: `/A` parses with `%u` into 16-bit `Uint4` globals, which only worked when the linker placed them in ascending address order. On this build they're descending, so `samprate` ends up 0. Since you're likely launching with `/A`, let me fix that too.

### Assistant

The fix is verified. Let me review the complete diff and do a final clean build/test.

### Assistant

Diagnosis confirmed, and it wasn't the filter itself.

## Root cause

The music uses the PWM/pulse-width path, and its pulses are often **shorter than one output sample**. A narrow pulse has near-flat harmonic energy right up to Nyquist, unlike the effects' square waves (whose harmonics fall off as 1/n). At 22.05 kHz those harmonics fold back into the audible band as noise.

I measured the same music driven by the real engine at 22.05 kHz vs 44.1 kHz (the latter is effectively alias-free). Energy above the 44.1 kHz reference:

| band | 22.05 kHz (before) | after fix |
|---|---|---|
| 3–4 kHz | +3.4 dB | −0.25 dB |
| 4–5 kHz | +5.2 dB | −0.34 dB |
| 5–6 kHz | +6.6 dB | −0.37 dB |
| 6–8 kHz | +7.2 dB | −0.42 dB |
| 8–10 kHz | +7.1 dB | −0.63 dB |

That's the distortion you were hearing — effects were mostly below it, music wasn't.

## Fix

- `newsnd.c` / `device.h`: added `snd_os`, the number of emulator samples per output sample. The emulator now always runs at **at least 44.1 kHz** internally (`snd_os` = 2 at 22.05 kHz output, 1 at 44.1 kHz, more at lower rates), so aliasing folds from a much quieter part of the spectrum.
- `sdl_snd.c`: `fill_audio()` generates `snd_os` samples per output sample through the speaker filter, averages them (decimation), then writes one output sample.
- `main.c`: fixed a pre-existing `/A` crash. It parsed with `%u` straight into 16-bit `Uint4` globals; on this linker layout that zeroed `sound_rate` and caused a divide-by-zero. It now parses into real `unsigned` temps. So `./digger /A:1,220,7,1,22050,512` works again.
- `newsnd.c`: guards against `samprate == 0` instead of dividing by zero.

## Verification

- Clean build, no new warnings.
- Music PSD now matches the 44.1 kHz reference within 0.6 dB across 300 Hz–10 kHz.
- Effect pitch and level are unchanged at 22.05 kHz vs 44.1 kHz (tested with the emerald sound: 690 Hz peak both ways).
- No clipping in music or the end-of-level tune (`min/max` 35/254, zero samples at 0 or 255).
- Headless runs succeed at 8000, 22050, 33000, and 44100 Hz, plus the INI path.

The changes are in the working tree but **not committed**. If it sounds right now, say the word and I'll add this exchange to `Worklog.md` and commit (per `Agents.md`).

### User

commit it

## Update Agents.md

Previous commit: f7422da

### User

Agents.md updated, commit it

### Assistant

Following the updated `Agents.md`: I'll add this exchange to the worklog, include the previous commit's hash (`f7422da`), and commit the `Agents.md` update together with the worklog.

## Add reverse-engineering artifacts and C workspace

Previous commit: cd23905

### User

Do it, your C decompilation progress can go into C/ (make a note in Agents.md)

### Assistant

I'll generate the annotated disassembly, data map, comparison report, and a `C/` decompilation workspace, then note the convention in `Agents.md`.

### Assistant

Done. The reverse-engineering artifacts are now under the repository root, and `Agents.md` records the convention.

## Created

```
.
├── ANALYSIS.md      comparison of the 1983 binary with the C port
├── SYMBOLS.md       code/data symbol map with confidence levels
├── digger.lst       full annotated 8086 disassembly (CS:IP addresses)
├── digger.dat       data-segment dump with DS offsets and table labels
└── C/
    ├── README.md    decompilation conventions and status
    ├── symbols.h    CS addresses, DS offsets, original-width types
    ├── sound.c      timer ISR, soundint, music, musicupdate,
    │                soundemerald, soundddie (reconstructed)
    ├── levels.c     getlevch + the 8 level arrays (verified from binary)
    ├── game.c       score/bonus fields and notes
    └── TODO.md      RNG, game logic, remaining sound helpers, graphics
```

`Agents.md` now ends with:

```
C decompilation progress goes in C/
```

## Key results captured

- **Exact matches**: all 8 level layouts, background jingle, dirge, bonus-jingle `T1` and `T2`, core UI strings, the `soundint` call order, the timer/speaker ISR, `music`, `musicupdate`, `soundemerald`, `soundddie`, and `getlevch`.
- **Bonus jingle**: original is `T1 (DS:0x45E4, 86 words)` + 112-byte embedded default-score block + `T2 (DS:0x4700, 179 words)`. The port inserts a 56-word bridge between `T1` and `T2`; both original sections match exactly.
- **Sound timing**: the ISR's 16-bit accumulator gives an 18.2 Hz effective `soundint`; `T1` is 21.4 s, so it is interrupted before reaching the score block when bonus mode ends.
- **Port additions**: VGA/EGA, SDL, SoundBlaster/DMA, recording, gauntlet, simultaneous 2-player, custom levels, and the `getlevch` simultaneous-mode `'H'` special case.
- **Open**: the original RNG algorithm (the port's `0x15A4E35` LCG is not in the binary), the remaining game-logic functions, and the uncompiled `sett0`/`setsoundmode`/`setsoundt2`/`sett2val` bodies.

Everything is left uncommitted. There is a `.ANALYSIS.md.swp` vim swap file in the tree which I did not touch. When you want it committed, I'll add this exchange to `Worklog.md` with the previous commit hash (`cd23905`) and commit the artifacts.

### User

commit it

## Complete sound hardware layer and map core game-state functions

Previous commit: c6fd3f8

### User

Continue the decompiling and comparison

### Assistant

I'll continue with the sound hardware layer first, then map the game loop and RNG, updating the reverse-engineering artifacts as I go.

### Assistant

The sound helpers expose a potential timing discrepancy, so let me verify the port's actual music tempo against the original's accumulator constants before writing it up.

### Assistant

The counter shows ~69–72 Hz, not 18.2 — the port's `timercount -= 0x4000` after each carry is what reproduces the original's `4*t0val` accumulator. Let me confirm over 5 s, then correct the analysis.

### Assistant

The sound layer is now fully mapped. Next I'll try to identify the original RNG by inspecting the 16-bit multiply/divide routines that could be `randno`.

### Assistant

The bonus timer constant isn't the `0xFA` hits (those are score additions). Let me finish the sound-helper decompilation and capture the score-value call sites.

### Assistant

Excellent: the high-call-count region contains `incpenalty`, `levplan`, `levof10`, `levno`, `getlives`, `addlife`, and `curplayer` — all matching port functions. Let me continue through that region to find the RNG.

### Assistant

The new functions need to appear in the disassembly map too. Let me regenerate `digger.lst` and `digger.dat` with the completed symbol set.

### Assistant

Continued the decompilation and comparison. The sound layer is now complete, the core state layer is mapped, and one timing result from the previous pass was corrected.

## Corrected timing result

The earlier “18.2 Hz” conclusion was wrong. `sett0` (`CS:0x7DD9`) stores **`4*t0val`** in the ISR accumulator (`DS:0x024A`), so with the timer-0 IRQ rate of `1,193,181/t0val` the `soundint` rate is:

```
4 * 1,193,181 / 65,536 ≈ 72.8 Hz
```

The port reaches the same rate differently: `timerrate=t0val` plus `timercount -= 0x4000` after every carry in `newsnd.c:getsample()`, giving `1,193,181/16,384 ≈ 72.8 Hz`. I instrumented the port: **360 `soundint` calls in 5 s (72.0 Hz)**. Corrected jingle durations:

| Tune | Notes | Duration @72.8 Hz |
|---|---:|---:|
| original `T1` | 43 | 5.4 s |
| original `T2` | 89 | 12.4 s |
| port's merged bonus jingle | 160 | 21.1 s |

## Completed sound layer

Reconstructed in `C/sound.c` from the binary:

- `sett0` (`CS:0x7DD9`) — timer-0 divisor, 1000-tick clamp, `4*t0val` accumulator, `musvol`/`pulsewidth` clamp, `setsoundmode`.
- `setsoundt2` (`CS:0x7E7C`), `setsoundmode` (`CS:0x7EB8`) — same `spkrmode`/`soundt0flag` logic and port `0x61` speaker-gate writes.
- `sett2val` (`CS:0x7FA2`), `timer2` (`CS:0x7FD5`) — timer-2 divisor writes.
- `startint8` (`CS:0x7EFF`), `stopint8` (`CS:0x7F36`) — INT 8 hook/restore and timer 0 programming.
- `soundoff` (`CS:0x7CC9`, probable), `initsound` (`CS:0x7D10`, probable), `soundpause`/`soundpauseoff` (`CS:0x7CB3`/`CS:0x7CBE`).

## Game-state layer

New `C/state.c` reconstructs the high-call-count state functions, all matching the port:

- `incpenalty` (`CS:0x8A32`, **52 callers**) → `DS:0x5014++`.
- `levno` (`CS:0x8ABD`) → level at `DS:0x4FE2 + 8*curplayer`.
- `levplan` (`CS:0x8A65`) → `(l&3)+5` for `l>8`.
- `levof10` (`CS:0x8A8D`) → `min(level,10)`.
- `getlives` (`CS:0x8AF0`), `addlife` (`CS:0x8B0C`) → lives fields at `DS:0x4FE0`/`DS:0x4FE8`.
- `curplayer` getter (`CS:0x8B45`), probable `nplayers` getter (`CS:0x8B4D`), `testpause` flow (`CS:0x8B55`).

Scoring is also confirmed:

- `score_add` (`CS:0x5FA1`) has the 999999 wrap, next-bonus step of `0x4E20`, and three `incpenalty` calls — matching `scores.c:addscore`.
- Wrappers at `CS:0x68A2..0x68DA` add **250 / 25 / 250 / 500 / 1000**, exactly matching `scorekill`, `scoreemerald`, `scoreoctave`, `scoregold`, `scorebonus`. The port’s `scorekill2` (125+125) is a simultaneous-mode addition.

## RNG status

Still open, but narrowed. The binary contains:

- no `0x15A4E35` (or common 16-bit LCG constants);
- no `idiv word [bp+disp]`;
- the long-multiply helper is called only once (string-to-number conversion);
- the long-divide helper only from number formatting.

So the original is likely a **16-bit/inlined generator or a table**, not the port’s 32-bit LCG. The next step is tracing calls from the monster/fire update functions.

## Artifacts updated

- `ANALYSIS.md` — corrected timing, sound-helper matches, state-function matches, RNG search results.
- `SYMBOLS.md` — new verified/probable code symbols and game-state DS fields.
- `digger.lst` — regenerated with the full symbol set.
- `digger.dat` — regenerated with the new state-field labels.
- `C/sound.c` — completed sound hardware layer.
- `C/state.c` — **new**; core state accessors.
- `C/game.c` — `addscore` structure and score-wrapper values.
- `C/symbols.h`, `README.md`, `TODO.md` — updated status and addresses.

All changes are uncommitted. When you want them committed, I’ll add this exchange to `Worklog.md` with the previous commit hash (`c6fd3f8`) and commit.

### User

Commit it

## CGA video analysis: direct mode 4, PCjr R2, DOSBox diagnosis

Previous commit: c467021

### User

Continue on CGA code, an undocumented mode is used and that cause dosbox to display incorrectly, side by side comparison in dosbox_digger.png, once this is understood I can clone dosbox for to attempt fixing this.

### Assistant

I'll start by looking at the comparison image and locating both the original's CGA setup and the port's CGA implementation.

### Assistant

The image looks like a 16-colour CGA title screen rendered with the wrong geometry — likely the 160×100 'undocumented' CGA mode. Let me decode the original's exact mode and CRTC programming.

### Assistant

Found the CGA setup. The key is a 14-byte CRTC table at `DS:0x0255` written to ports `0x3D4/0x3D5`. Let me decode those registers and the surrounding mode control.

### Assistant

The registers decode as standard CGA mode 4 with a PCjr H-sync value (`R2=0x2C`) and disabled cursor — plus the game changes `R2` at runtime. Let me verify the archive build and DOSBox's mode handling.

### Assistant

The archive's working config is the important clue: `machine=cga`, `cycles=fixed 400`. Let me check DOSBox's CGA mode handling to explain the difference between CGA-only and EGA/VGA machines.

### Assistant

The screenshot is from the archive's `digdug.com`, which is a different 57,600-byte build from the repo's `digger.com`. Let me compare their CGA code and look at the archive's working screenshots.

### Assistant

The archive's screenshots confirm the game is standard 320×200×4 CGA; the garbling is the direct-register path. Let me decode the pixel/blit routines to nail down the exact framebuffer layout.

### Assistant

One artifact correction surfaced: the CGA messages are actually at `DS:0x0263`/`DS:0x0322`; the earlier `0x0195` labels were wrong. Let me verify and fix that, then document the CGA findings.

### Assistant

## Bottom line

It is **not a different resolution mode**. Digger is using **standard CGA mode 4: 320×200, 4 colours, 2 bits/pixel, interleaved banks**. What is “undocumented” is that it enters mode 4 by **writing the CGA registers directly** instead of calling INT 10h, and its CRTC table uses the **PCjr H-sync value `R2=0x2C`** (IBM CGA mode 4 uses `0x2D`). It also rewrites `R2` at runtime for a horizontal shift.

That is consistent with DOSBox leaving the display in text/VGA mode: the 2bpp title bitmap is then interpreted as 80×25 characters/attributes, producing the repeated text and green boxes in the left half of `dosbox_digger.png`.

## Exact setup from `digger.com`

The init entry is `CS:0x03F4` → `CS:0x03FC` → `cga_init` at `CS:0x0429`:

```
3D8h = 02h                 ; video off, graphics, colour, 320x200
CRTC R0..R13 = 38 28 2C 0A 7F 06 64 70 02 01 20 00 00 00
3D9h = 20h                 ; background black, palette 1
fill the 16KB framebuffer with colour 0
3D8h = 0Ah                 ; video on, graphics, colour, 320x200
```

CRTC differences from IBM mode 4: **`R2=0x2C`** (PCjr H-sync; IBM `0x2D`) and cursor registers `R10=0x20`, `R11=0x00` (cursor disabled). `R9=1` is the normal CGA mode 4/5/6 maximum-scan-line value, not the anomaly.

Framebuffer layout, from the pixel routine at `CS:0x05CF`:

```
addr = (y & ~1) * 40 + (y & 1) * 0x2000 + (x >> 2)
```

- 320×200, 80 bytes/scanline, 16KB at `B800:0000`.
- Even scanlines in bank 0, odd scanlines in bank 1 (`+0x2000`).
- Four pixels per byte, 2 bits each, leftmost in bits 7–6.
- The title screen is an RLE-compressed 16KB bitmap at `DS:0x0DAC`, blitted by `CS:0x17EB` (`0xFE count value`).

The runtime shift is in the keyboard handler at `CS:0x1128`/`CS:0x1155`: it increments/decrements `DS:0x0673` (initial value **0x2C**) and writes it to CRTC R2 (clamped 0x2A..0x2E). That is an H-sync shift, **not** a framebuffer/stride change.

## DOSBox behaviour

- `src/hardware/vga_other.cpp:write_cga()` handles `3D8h/3D9h` **only when `machine==MCH_CGA`**. `0x02`/`0x0A` map to `M_TANDY4` (CGA 320×200×4), then `write_color_select()`/`VGA_SetBlinking()`.
- On the **default EGA/VGA machine**, those direct CGA port writes are not handled by `write_cga()` at all. The VGA can remain in text/VGA mode while the game writes CGA CRTC values and a CGA framebuffer → the repeated text/green-box garbage. This is the VOGONS “direct 3D8” issue.
- On `machine=cga`, `VGA_SetupDrawing()` for `M_TANDY4` uses `vga.other.hdend/vdend/max_scanline`, `doubleheight=true`, `doublewidth=(mode_control & 1)==0`, a fixed 80-byte source stride, and the CGA line handler’s `line_mask`/`line_shift`/`addr_mask`. If the game’s direct CRTC R0–R13 writes are not propagated into `vga.other` / the CGA line state / a renderer resize, the renderer keeps stale timing — a 2×2 tiling like the screenshot is what stale text-mode geometry can produce. R2 must be treated as H-sync only, never as visible width/stride.

## Working reference

The Internet Archive item **Digger (working CGA)** uses:

```
[dosbox]
machine=cga
[cpu]
cycles=fixed 400
```

Its `screenshot_00.png` (title) and `00_coverscreenshot.png` (gameplay) show the correct 320×200×4 display. The left half of your `dosbox_digger.png` is the archive’s **`digdug.com`** build (hence the window title “Program: DIGDUG”). That build and `digger.com` have the **same CGA code and CRTC table at the same file offsets**; the binaries differ elsewhere (SHA-256 differs). Your DOSBox shows **3000 cycles**, not the archive’s 400, so the machine type and cycle setting are the first things to isolate.

## Fix directions

1. **Test the archive config first**: `machine=cga`, `cycles=fixed 400`. If the title/game displays correctly, no DOSBox code change is needed for your current setup.
2. **If you want the default EGA/VGA machine to work**: make the VGA/EGA `3D8h` path detect the CGA mode-control write and perform a CGA-compatible mode-4 switch (the equivalent of BIOS INT 10h mode 4) after the game’s CRTC writes, or emulate a CGA CRTC overlay. The game-level workaround is the VOGONS patch: at file offset `0x38F`, replace `BA D8 03 B0 02 EE` with `B8 04 00 CD 10 90`.
3. **If `machine=cga` still garbles**: audit `vga_crtc.cpp` (`write_crtc`/`VGA_StartResize`), `vga_draw.cpp` (`VGA_SetupDrawing`, `M_TANDY4`/`M_CGA4`, `address_add=80`, CGA line handlers), and the INT 10h teletype/scroll path used for the startup message. Ensure direct R0–R13 writes while video is off update the CGA timing/state, and that R2 only changes H-sync.
4. Compare with DOSBox-X’s `vga_other.cpp`/`vga_draw.cpp` and MAME’s CGA device; both have more complete CGA mode handling.

## Added to the repo

- `CGA.md` — full mode/register analysis, DOSBox code paths, fix directions.
- `C/cga.c` — reconstructed `cga_init`, palette/background/intensity, retrace wait, pixel address/apply, replace/XOR pixel, R2 shift, and RLE blit.
- Updated `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst`, `digger.dat`, `C/README.md`, `C/TODO.md`, `C/symbols.h`.
- Corrected the CGA message offsets: they are `DS:0x0263` and `DS:0x0322` (file `0x8FD3`/`0x9092`); the earlier `0x0195` labels were wrong.

All changes are uncommitted, and `dosbox_digger.png` is still untracked. When you want it committed, I’ll add this exchange to `Worklog.md` with the previous commit hash (`c467021`) and include the CGA artifacts (and the comparison PNG if you want it tracked).

Sources:
- [VOGONS: Digger and Hercules Graphics, using CGA emulation](https://www.vogons.org/viewtopic.php?t=33424)
- [Internet Archive: Digger (working CGA)](https://archive.org/details/digger_cga)
- [Internet Archive: Digger (with palette fix)](https://archive.org/details/digger_with_palette_fix)
- [reenigne: 8088/CGA register values](https://github.com/reenigne/reenigne/blob/master/8088/cga/register_values.txt)
- [DOSBox vga_other.cpp](https://github.com/blackberry/DosBox/blob/master/src/hardware/vga_other.cpp)
- [DOSBox-X vga_other.cpp source](https://dosbox-x.com/doxygen/html/vga__other_8cpp_source.html)
- [int10h.org: Astro-Dodge's Dirty Video Tricks](https://int10h.org/blog/2023/10/astro-dodge-cga-dirty-video-tricks/)

### User

commit it

## Finish high-priority tasks: RNG, game-loop map, bonus timer, score I/O

Previous commit: fc9f41d

### User

Finish the high priority tasks

### Assistant

I'll finish the high-priority items: RNG, game-logic mapping, bonus timer, and score I/O. Starting with the score path and the disk routines near the start of the binary.

### Assistant

No INT 21h at all — the original stores scores in a raw disk sector via INT 13h. Let me trace the sector-read/write callers and the score buffer.

### Assistant

Score I/O is now solved: the original uses INT 13h sector 7, not a DOS file. Let me read the port's bonus-mode code and locate the matching original routine.

### Assistant

The port's `initbonusmode` calls `levof10()`; I can find the original by its callers. Let me trace `levof10` and the CGA intensity calls.

### Assistant

The RNG is now solved: a 256-byte permutation table plus the BIOS tick counter, shared by game and sound. Let me decode the two game call sites and the title/game loop.

### Assistant

The RNG is confirmed as a table+BIOS-tick generator. Let me finish the game-logic mapping by locating the update function callers and the main game loop.

### Assistant

The RNG and monster functions are mapped. Let me locate the main game loop via the `domonsters` callers and the post-title setup.

### Assistant

The high-priority paths are all identified. Let me record them: RNG, game-loop map, bonus timer, and sector-based score I/O.

### Assistant

One final check: confirm `dodigger` contains the bonus-mode block by tracing the bonus variables.

### Assistant

All four high-priority items are now resolved. Let me run the final consistency checks.

### Assistant

The four high-priority items are finished. Everything is recorded in the reverse-engineering files; the only remaining high-priority work is now the line-level movement/collision/drawing bodies.

## 1. RNG — solved

The original has **one shared generator** at `CS:0x177F`, used by both the game and the sound engine:

```
seed = ++DS:0x0DA7;
tick = byte at 0x40:0x006C;              (BIOS timer low byte)
r    = DS:0x0CA7[(seed + tick) & 0xFF];  (256-byte permutation table)
return (r * (n + 1)) >> 8;
```

- The table at `DS:0x0CA7` is a permutation of all 256 byte values (verified against the binary).
- Call sites: `domonsters` (`randno(15-levof10())`), `monai` (`randno(levof10()+5)`), and `soundfireupdate`.
- The port’s `randno`/`randnos` 32-bit LCG (`0x15A4E35`) is a replacement; its sequence differs. On SDL, `getlrt()` returns 0, so the port is deterministic.

Reconstructed in `C/rng.c` (table + function).

## 2. Game logic — mapped

The main loop `CS:0x86F1` matches the port’s `game()`:

| Function | Address | Confidence |
|---|---|---|
| `game` loop | `CS:0x86F1` | verified structure |
| `dodigger` | `CS:0x4939` | verified (contains the bonus block) |
| `domonsters` | `CS:0x2C90` | verified (six monsters, stride 0x28, `hnt > 10-levof10()`, `randno(15-levof10())`) |
| `monai` | `CS:0x2E75` | verified (`randno(levof10()+5)==1` direction swap) |
| `dobags` | `CS:0x3E2D` | probable |
| `initlevel` / `drawscreen` / `initchars` / `checklevdone` | `0x89A4` / `0x89CD` / `0x89E4` / `0x89F2` | probable |
| `incmont`, `testpause`, `getnmovingbags`, `countem`, `readdir` | `0x3AEF`, `0x8B55`, `0x47F8`, `0x5A94`, `0x5BB9` | mapped |

The per-frame order and the post-level path (`getnmovingbags` loop, soundstop, erasebonus, cleanupbags, savefield, erasemonsters, countem, next level) match the port. The detailed movement/collision/drawing bodies remain to be reconstructed line-by-line.

## 3. Bonus timer — solved

`initbonusmode` at `CS:0x552A` is:

```
bonusmode = 1;
erasebonus();
ginten(1);
bonustimeleft = 250 - levof10()*20;   (DS:0x400E)
startbonustimeleft = 20;              (DS:0x400C)
DS:0x4014 = 1;
```

The port’s `250 - levof10()*20` is **original**, not a port addition. `createbonus` (`CS:0x5510`), `endbonusmode` (`CS:0x5562`), and `erasebonus` (`CS:0x5575`) match the port. Reconstructed in `C/game.c`.

## 4. Score I/O — solved

There is **no INT 21h anywhere**. High scores live in raw disk sector 7:

```
drive 0, head 0, cylinder 0x27 (39), sector 7, 1 sector
buffer ES:BX = DS:0x0046  (512 bytes)
```

- `read_sector7` `CS:0x028F`: INT 13h AH=2.
- `write_sector7` `CS:0x02B8`: INT 13h AH=4 verify, then AH=3 write, 3 retries.
- `loadscores` `CS:0x5DED`: reads sector 7 and parses `'s'` + ten `[3 initials][2 bytes][6 digits]` entries into `DS:0x4134`/`DS:0x4108`.
- `savescores` `CS:0x6521`: builds the same 512-byte buffer and writes sector 7.
- The format is identical to the port’s `.sco`.
- The 112-byte block at `DS:0x4690` (score 1200) has no direct code reference; it is a default/legacy image. The sector-2 read/check at `CS:0x0321`/`0x02F3` is disk signature/copy protection.

## Artifacts updated

- `C/rng.c` — **new**: 256-byte table + RNG reconstruction.
- `C/game.c` — expanded: game-loop map, bonus functions, score I/O, RNG notes.
- `ANALYSIS.md` — new RNG, main-loop, bonus, and score-storage sections; RNG difference updated.
- `SYMBOLS.md` — new code/data symbols and confidence levels.
- `digger.lst` / `digger.dat` — RNG/game/score/bonus labels.
- `C/symbols.h`, `README.md`, `TODO.md` — status updated.

All changes are uncommitted. `C/.TODO.md.swp` (a vim swap file) and `C/rng.c` are untracked. When you want it committed, I’ll add this exchange to `Worklog.md` with the previous commit hash (`fc9f41d`) and commit.

### User

Commit and continue with game logic

### Assistant

Continued the game-logic reconstruction and synchronized the artifacts.

## Newly verified/reconstructed

**`monai` movement branch (CS:0x306A onward)** — the "remaining" part is now fully mapped and matches the port step for step:
- screen-edge stop → `dir = DIR_NONE`
- `hdir` update for horizontal moves
- `eatfield`, then the direction switch with `drawrightblob`/`drawtopblob`/`drawleftblob`/`drawbottomblob` (CS:0x21CC/0x225C/0x2214/0x22A2, verified by their `x±`, `y±` and sizes)
- movement deltas `x±4`, `y∓3`, `hitemerald`, `digalive`, `stime`, `hnt`
- `drawmon`, monster/bag/digger collision blocks, and the final `h/v/xr/yr` update

**Monster module** (`C/monster.c`, rewritten):
- full `domonsters` including the spawn timer (`nextmontime`, `createmonster`) and `unbonusflag` → `createbonus`
- `initmonsters` (0x2BC1), `erasemonsters` (0x2C58), `createmonster` (0x2DD3), `mondie` (0x3682), `fieldclear` (0x3815), `nmononscr`, `incmont`, `mongold`
- collision helpers 0x3767/0x3795

Notable findings: the original has **no per-monster `chase` field** — it always chases digger 0; and `createbonus` ignores the extra word its caller pushes.

## Corrected field maps

- `0x3FDA` = `pushcount`, `0x3FDC` = global `goldtime` (`150 - levof10()*10`), not bag fields.
- Monster record extends to `0x3B3C`: `hdir 0x3B2C`, `death 0x3B36`, `bag 0x3B38`, `dtime 0x3B3A`, `stime 0x3B3C`; the record is 0x28 bytes with three unreferenced trailing words.
- Monster scheduling globals `0x3C0C..0x3C18` and digger `death/bag` at `0x4002/0x4004`.
- `CS_CREATEMONSTER` was mis-typed as `0x3DD3`; fixed to `0x2DD3`.

Updated `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst` (21 new labels), `digger.dat`, `C/symbols.h`, `README.md`, `TODO.md`. All `C/*.c` pass `gcc -fsyntax-only -std=c99 -Wall -Wextra`.

Nothing committed for this batch yet; `C/.TODO.md.swp` is left untouched per `Agents.md`. Next in the queue is `updatefire` (0x4A3E), which I've started reading — it is a large routine with its own fireball state at `0x3FEE..0x4000`.

## Reconstruct the full monster module and finish `monai`

Previous commit: a5ce437

### User

Commit and continue

### Assistant

Committed the game-logic continuation as `ae357ba` (12 files, +1228/−23), then kept going.

**Commit `ae357ba` — “Reconstruct the full monster module and finish monai”** (previous `a5ce437`, worklog included):
- full `monster.c`: `domonsters` spawn logic, complete `monai`, `createmonster`, `mondie`, `initmonsters`, `erasemonsters`, `fieldclear`, collision helpers, `nmononscr`/`incmont`/`mongold`
- new `bags.c`, `digger.c`, `drawing.c`
- artifact/label sync across `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst`, `digger.dat`, `symbols.h`

**Continued after the commit — fireball path (`C/digger.c`):**
- `updatefire` (`CS:0x4A3E`): launch with `levof10()*3+60` recharge, per-direction `fx/fy`, sprite 15, 7-pixel `ggetpix` probes, `drawfire`, `killmon`/`scorekill` monster hits, the `0x40FE` impact mask, and the edge/impact explosion tests
- `drawexplosion` (`CS:0x4E66`) and `killfire` (`CS:0x4EE3`)
- verified `movedrawspr` `0x07B1`, `erasespr` `0x07D3`, `drawfire` `0x24C6`, `ggetpix` `0x56A6`, `getfirepflag` `0x5DD8`, `soundfire` `0x7571`, `soundfireoff` `0x7582`, `soundexplode` `0x75E6`
- corrected the digger field map: `ddir` `0x3FEC`, `dir` `0x3FEE`, `fx/fy/fdir` `0x3FFA..0x3FFE`, `deathstage`/`deathbag` `0x4002/0x4004`, `emn` `0x400A`; `firepressed` `0x0663` is distinct from `getfirepflag`'s `DS:0x40BC`

All `C/*.c` pass `gcc -fsyntax-only -std=c99 -Wall -Wextra`. Uncommitted: the updatefire batch plus artifact updates (8 files); `C/.TODO.md.swp` untouched. Commit is on `main`, one ahead of `origin/main`, not pushed. Next: `updatedigger` (`CS:0x4F07`) and `diggerdie` (`CS:0x5218`).

## What was added

- **Full `monai`** (`CS:0x2E75`): direction selection (priority order, `bonusmode` reversal, `reversedir`, random swap, `fieldclear`), screen-edge stop, `hdir`, `eatfield`, blob drawing and the ±4/∓3 movement, `hitemerald`, `digalive`/`stime` rollback, `hnt`, `drawmon`, monster/bag/digger collisions, coordinate update.
- **Monster scheduling** in `domonsters` (`CS:0x2C90`): `nextmontime`, `createmonster`, and the `unbonusflag` → `createbonus` step.
- **Helper routines**: `initmonsters` (`0x2BC1`), `erasemonsters` (`0x2C58`), `createmonster` (`0x2DD3`), `mondie` (`0x3682`), `nmononscr` (`0x3ABB`), `incmont` (`0x3AEF`), `mongold` (`0x3B26`), collision routines `0x3767`/`0x3795`, `fieldclear` (`0x3815`), blob draws `0x21CC`/`0x2214`/`0x225C`/`0x22A2`, `killmon` (`0x39A9`), `killdigger` (`0x5668`), `bagmask` (`0x4774`), `removebags` (`0x47B5`).
- **Corrections**: `pushcount` `0x3FDA` / global `goldtime` `0x3FDC`; monster record fields through `stime` `0x3B3C`; scheduling globals `0x3C0C..0x3C18`; digger `death`/`bag` `0x4002`/`0x4004`; `CS_CREATEMONSTER` fixed to `0x2DD3`.

Artifacts: `C/monster.c` (rewritten), `C/digger.c`, `C/bags.c`, `C/drawing.c`, `C/symbols.h`, `C/README.md`, `C/TODO.md`, `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst`, `digger.dat`.

## Fireball path: `updatefire`, `drawexplosion`, `killfire`

Previous commit: ae357ba

### User

Commit and continue

### Assistant

Committed the fireball reconstruction and continued with `updatedigger`.

## What was added

- **`updatefire`** (`CS:0x4A3E`): the not-firing branch decrements `rechargetime`, checks `getfirepflag` (`DS:0x40BC`) and `digalive`, sets `rechargetime = levof10()*3 + 60`, clears `notfiring`, computes `fx`/`fy` from `dir` (right `x+8,y+4`; up `x,y+4`; left `x+4,y`; down `x+4,y+8`), copies `fdir = dir`, draws sprite 15 and calls `soundfire`. The firing branch switches on `fdir`, advances by 8 (horizontal) or 7 (vertical), probes the CGA pixels with `ggetpix`, calls `drawfire` for frame 0, then handles monster hits (`killmon` + `scorekill` + `expsn=1`), the `0x40FE` impact mask, and the edge/impact explosion with the reverse 8/7 step and a redraw.
- **`drawexplosion`** (`CS:0x4E66`): frames 1/2/3 draw and increment `expsn`, `soundexplode` on frame 1, `killfire` + `expsn=0` otherwise.
- **`killfire`** (`CS:0x4EE3`): sets `notfiring`, `erasespr(15)`, `soundfireoff`.
- **Verified helpers**: `movedrawspr` `0x07B1`, `erasespr` `0x07D3`, `drawfire` `0x24C6` (returns the collision mask), `ggetpix` `0x56A6`, `getfirepflag` `0x5DD8`, `soundfire` `0x7571`, `soundfireoff` `0x7582`, `soundexplode` `0x75E6`.
- **Field map**: `ddir` `0x3FEC` (key direction, a port local), `dir` `0x3FEE`, `fx` `0x3FFA`, `fy` `0x3FFC`, `fdir` `0x3FFE`, `deathstage` `0x4002`, `deathbag` `0x4004`, `emn` `0x400A`; `firepressed` `0x0663` is the input latch, while `getfirepflag` reads the separate byte at `DS:0x40BC`.

Artifacts: `C/digger.c`, `C/symbols.h`, `SYMBOLS.md`, `ANALYSIS.md`, `C/README.md`, `C/TODO.md`, `digger.lst`, `digger.dat`, `Worklog.md`.

### User

Finish game bodies

### Assistant

Game-state bodies are finished. Everything compiles cleanly with `-Wall -Wextra`; the batch is uncommitted.

**`diggerdie` (`CS:0x5218`)** — full state machine: bag settle, frames 14→10, the 3/5/6/6/5/3/0 sinking arc, `setdead(1)`. Key difference from the port: the original does **not** deduct lives or respawn here. State 4 only sets the per-player dead flag (`0x4FDE + 8*curplayer`); the main loop (`CS:0x8936`) does `lives--`, `drawlives`, and the game-over check.

**Bag module (`C/bags.c`, rewritten)** — `initbags`, `drawbags`, `updatebag`, `baghitground`, `pushbag`, `pushbags`, `pushudbags`, `removebag(s)`, `getgold`, `bagy`, `getbagdir`, `bagmask`, `getnmovingbags`. Verified full record layout: `x,y,h,v,xr,yr,dir,wobbling,wt,unfallen,exist,gt,fallh`; saved arrays at `0x3C1A`/`0x3D5A`. Notes: vertical pushes only succeed on gold bags, and `pushbag` tests the raw digger/monster collision bits rather than the port's `digalive()` loop.

**Field/emerald (`C/field.c`, new)** — `eatfield` (field words at `DS:0x4438`, stride `0x1E`; horizontal bits `0x1F`, vertical `0xFC0`, solid `0x2000`), `hitemerald` (`embox[8]={8,12,12,9,16,12,6,9}`, byte map at `DS:0x4018`), `killemerald`, `countem`.

**Monster helpers (`C/monster.c`)** — `killmonsters`, `squashmonster(s)`, `checkmonscared`, `monleft`; plus `checkdiggerunderbag` in `digger.c`.

Artifacts updated: `SYMBOLS.md` (function/data tables, bag/field layouts), `ANALYSIS.md`, `C/README.md`, `TODO.md`, `symbols.h`, 10 new `.lst` labels, `digger.dat` field annotations.

Remaining is now only rendering: `drawmon`/`drawmondie`/`drawdig`/`drawgold`, `makefield`/`makeemfield`, `drawstatics`/`drawemeralds`, the blob draws, and `initmbspr`/`creatembspr`. Both `.swp` files are untouched. Say the word and I'll commit this batch with the worklog entry.

## Digger death, bag physics, and field helpers

Previous commit: 59dac52

### User

Commit and finish rendering task

### User

Commit it

### Assistant

Committed the game-state bodies (11 files): `diggerdie`, the full bag module, the field/emerald helpers, and the remaining monster helpers. All `C/*.c` compile cleanly with `-Wall -Wextra`. The rendering routines are next.

## What was added

- **`diggerdie`**: states 1..5, frames 14..10, fall arc, `setdead(1)`. Life loss and respawn are in the main loop, not here.
- **Bags**: `initbags`, `drawbags`, `updatebag`, `baghitground`, `pushbag`, `pushbags`, `pushudbags`, `removebag(s)`, `getgold`, `bagy`, `getbagdir`, `bagmask`, `getnmovingbags`; full 0x28-byte record layout and the two saved arrays.
- **Field**: `eatfield` (bit layout), `hitemerald` (`embox`), `killemerald`, `countem`.
- **Monsters**: `killmonsters`, `squashmonster(s)`, `checkmonscared`, `monleft`; `checkdiggerunderbag`.

Artifacts: `C/bags.c`, `C/digger.c`, `C/monster.c`, `C/field.c`, `C/symbols.h`, `C/README.md`, `C/TODO.md`, `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst`, `digger.dat`, `Worklog.md`.

## Rendering layer: draws, sprites, field and background

Previous commit: 533ba12

### User

Commit and finish rendering task

### Assistant

Committed the rendering layer (10 files): the draw functions, sprite-buffer initialisers, background tiler, and the field/emerald renderers. All `C/*.c` compile cleanly with `-Wall -Wextra`. Only the low-level pixel blitters and the CGA sprite data remain.

## What was added

- **Sprite initialisers**: `creatembspr` (`0x1826`, third frame per sprite), `initmbspr` (`0x1A03`, default frames), `createdbfspr` (`0x20F6`), `initdbfspr` (`0x2167`, animation reset).
- **Actors**: `drawmon` (`0x1B84`, per-monster leg state `0x16CC/0x16D8`, hobbin and nobbin right/left frame sets), `drawmondie` (`0x1DDB`), `drawdig` (`0x263F`, four directions x three legs x fire/no-fire, plus death frames 10..15), `drawgold` (`0x1E9D`, seven 4x15 frames), `drawbonus` (`0x260E`), `drawfire` (`0x24C6`, three animated legs and three explosion frames).
- **Field and background**: `makefield`, `savefield`, `drawfield`, `makeemfield`, `drawemeralds` in `field.c`; `drawstatics` restores the saved field and calls `setretr`/`gpal`/`ginten`/`drawbackg`/`drawfield`; `drawbackg` (`0x237A`) tiles eight 5x4 tile sets over the 320x186 background.
- **Misc sprites**: `drawemerald`/`eraseemerald` (4x10), `drawrightblob`/`drawleftblob` (2x18), `drawtopblob`/`drawbottomblob` (6x6), `drawfurryblob` (6x8), `drawsquareblob` (6x6).
- **Mapped primitives**: `setspr` `0x0804`, `setspr2` `0x0754`, `drawspr` `0x07E4`, `initmiscspr` `0x084F`, `drawmiscspr` `0x08AD`, `getis` `0x0876`.

Artifacts: `C/drawing.c`, `C/field.c`, `C/monster.c`, `C/symbols.h`, `C/README.md`, `C/TODO.md`, `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst`, `digger.dat`, `Worklog.md`.

### User

continue

### Assistant

Continued into the low-level layer: the sprite engine and pixel blitters are now reconstructed in a new `C/sprite.c` (uncommitted; all files compile cleanly with `-Wall -Wextra`).

**Sprite engine**
- 16 sprites + one misc slot, with the property arrays at `DS:0x03CF..0x0603` (`sprrdrwf`, `sprrecf`, `sprenf`, live/nominal sources, boxes, sizes) and the default draw order `0..15` at `DS:0x0627`.
- `setspr`/`setspr2`/`setorder`, `movedrawspr`, `erasespr`, and the two-stage `drawspr` (`0x07E4` → `0x0997`).
- Dirty-region recursion `clearrdrwf`/`clearrecf`/`setrdrwflgs`, the box tests `collide`/`bcollide`, and `bcollides` filling `first[5]`/`coll[16]`.
- Flush paths `putis`/`putims`/`getis`, plus `initmiscspr`, `initmiscspr2` (returns the redraw mask), and `drawmiscspr`.

**Pixel layer**
- `ggeti` (screen → saved buffer), `gputi` (restore), `gputim` (`(screen & mask) | data`), `gputich` (glyph path), all using the CGA interleaved address `(y&~1)*40 + (y&1)*0x2000 + x/4` and the even/odd bank step.

**Notable original-vs-port difference:** the original's `setrdrwflgs` and `bcollides` retry the overlap test with the candidate sprite shifted by `+320/-2` and `-640/+4` to catch screen-edge wrapping; the port's version doesn't do this. That is reproduced in the reconstruction.

Artifacts updated: `SYMBOLS.md` (sprite arrays, function table), `ANALYSIS.md`, `C/README.md`, `TODO.md`, `symbols.h`, 22 new `.lst` labels, and `digger.dat` annotations.

Remaining: the title-screen/menu code (`CS:0x82A6` and its glyph drawer `0x08E5`), the INT 10h teletype/scroll path, and decoding the sprite/tile data at `CS:0x16E4..0x3B00` and the title bitmap. Both vim swap files were left untouched.

## Sprite engine and pixel layer

Previous commit: ea0a20a

### User

Commit it, then finish the title-screen/menu code

### Assistant

Committed the sprite engine and pixel layer (9 files). Continuing with the title-screen/menu code.

## What was added

- **Sprite engine** (`C/sprite.c`): 17-entry dirty/recursion flags, 16 sprites plus the misc slot, live/nominal source pairs, boxes and sizes, the draw-order table, `setspr`/`setspr2`/`setorder`, `movedrawspr`/`erasespr`/`drawspr`, `clearrdrwf`/`clearrecf`/`setrdrwflgs`, `collide`/`bcollide`/`bcollides`, `putis`/`putims`/`getis`, the misc-sprite routines, and the CGA pixel blitters `ggeti`/`gputi`/`gputim`/`gputich` with the interleaved bank step.
- **Original-vs-port difference**: the edge-wrapping retries (`+320/-2`, `-640/+4`) in `setrdrwflgs`/`bcollides` are present in the original and reproduced here.

Artifacts: `C/sprite.c`, `C/symbols.h`, `C/README.md`, `C/TODO.md`, `SYMBOLS.md`, `ANALYSIS.md`, `digger.lst`, `digger.dat`, `Worklog.md`.
