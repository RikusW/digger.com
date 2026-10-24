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
