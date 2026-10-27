# Pipo Loop — test report

Everything below was observed in rytm-emu on the named build. Nothing has run on a unit yet.

- Source (0.1.0-experimental): `build.json` `bf75ec1a…512a`, `src/audio.s` `51cc6df6…8996`,
  `src/pipo.c` `40dcb80f…c466`. Fingerprint (`sha256` of `sha256sum build.json src/audio.s src/pipo.c`):
  `2b82007f…acf8` (`evidence.sourceSha256`).
- Stock OS 1.74: `.syx` `cb0f99d6…e123`, the owner's file.
- Core: elekloader core-ar2 0.3 (`core-ar2-0.3.elemod` `6b97dfb3…3e58`, as vendored).
- Build PP06 (core-ar2 0.3 + Pipo Loop 0.1.0-experimental, Modwerk's linker, `--version PP06`), from
  that source: `.syx` `0da96191207ddb17ccf2f6b824cf20a968b3375408f7a8071ffc286a269f33c3`, main OS `319e9355…85a6`.
- Build PC04 (core-ar2 0.3 + Silent Pad+Trig 0.5.1 + Pipo Loop 0.1.0, `--version PC04`):
  `.syx` `fa6342c47f843f019382720cc98818ace3240a7c0b95d36f343397414a199e70`. The same pipeline builds
  Silent Pad+Trig alone byte for byte as its PS08 (`ff7c4cf3…43d4`).
- Build PX04 (core-ar2 0.3 + Pipo Loop, Filter Hz, Loop Sync and Silent Pad+Trig): `.syx` `e96ff14c…1dfe`.
  Linked only, not booted.
- Earlier builds: PP03 and PC01 (before the code review), PP05 and PC03 (after it, before the three-position
  switch); each passed the battery below.
- Emulator: rytm-emu `c5732f0`, headless `dtboot`, JIT on. A card image holds one generated sample,
  RAMP (24,000 frames, a straight rise from -26,000 to +26,000), loaded to the project and set on
  BD with LOP ON on the SAMPLE page (the setup in [media/capture.json](media/capture.json)). Audio is
  dtboot's `--wav` with `--every 20M` (longer report intervals keep only about a second of audio).
  Effective LOP is read from the track's effective parameter word (`DTBOOT_PEEK`).

## Emulator, 0.1.0 (2026-10-27)

Every row was run on PP06, except the MIDI row (PP03); PP03 and PP05 gave the same results.

| Check | Stock 1.74 | PP06 |
| --- | --- | --- |
| LOP turned clockwise from ON | stays ON | reaches PIPO; the value text reads PIPO |
| LOP turned +4, -6, +4 detents from ON | — | ON, PIPO, ON, OFF, ON, PIPO |
| LOP's switch at OFF / ON / PIPO | two-way: knob left / right | three-way: knob left / middle / right, the same pictures as BD CLASSIC's WAV at 0 / 1 / 2 on stock |
| BD PLASTIC's TYP (another OFF/ON switch) | two-way | two-way, as stock |
| Ramp, LOP ON, one BD hit | sawtooth, period 24,000 frames, a 12,044 step at each wrap | identical |
| Ramp, LOP PIPO, one BD hit | (no PIPO) | triangle; turns every 23,935-23,937 frames; largest sample-to-sample step 5 (of 32,767) |
| PIPO at TUN +24 / about -24 | — | triangle, segments 5,952 frames (6,000 ideal) / 95,872 (96,000), largest step 5 |
| LOP turned ON to PIPO while BD sounds | — | saw until the knob reaches PIPO, then it plays on forward and turns at END; no early turn |
| P-lock: GRID RECORDING, trigs on steps 1 and 9, LOP locked to PIPO on step 1, PLAY | — | lock shows PIPO; effective LOP alternates PIPO (step 1) and ON (step 9); triangle from step 1, saw from step 9 |
| Retrig while playing backward (step 9 above) | — | restarts forward from STA |
| Scene: SCENE held, BD pad held, LOP turned | — | footer SCENE: 1, LOP locked to PIPO; toggling scene 1 in SCENE mode switches effective LOP between ON and PIPO |
| Two voices: BD and SD both RAMP, LOP PIPO, hit together | — | both channels ping-pong (BD clean; SD's channel also carries its analog snare) |
| MIDI CC 30 on channel 1: 0, 32, 63, 64, 85, 86, 127, 0, 1, 2 (PP03) | LOP = the CC value, unclamped | identical (so CC 2 gives PIPO) |
| PC04: ramp with LOP ON / PIPO | — | saw / triangle, as PP06 |

The turn needs one block of lookahead: with none (tested by setting it to 0), stock wraps the voice
to STA first and the saw comes back. The prototype looked two blocks ahead (turns 23,872 frames
apart); one block halves the early turn and stays click-free at every pitch above.

### Saving and stock compatibility

- PP06 with BD on LOP PIPO, power-off (`--power-off`; the firmware's handler saved the working state):
  the card then cold-booted on **stock 1.74** with effective LOP 2 (no clamp) and a BD hit played a
  **normal forward loop** (sawtooth, period 24,000, as ON).
- The same card cold-booted on PP06: LOP PIPO came back and BD ping-ponged.

### Captures

`media/lop-off.png`, `media/lop-on.png`, `media/lop-pipo.png`, `media/plock-pipo.png` and
`media/scene-pipo.png`: PP06 frames, plans in [media/capture.json](media/capture.json).

## Hardware

### PC03 on an Analog Rytm MKII (2026-10-27), as reported

npp1993 flashed PC03 (`f128beb4…1e2a`; core-ar2 0.3, Silent Pad+Trig 0.5.1, Pipo Loop 0.1.0) and reported:

- LOP displays the three values OFF, ON and PIPO, but the switch graphic is only two-way: PIPO draws like ON.
- Zero crossing does not work. (Not part of this module: stock never snaps STA, END or the loop to a zero
  crossing, and Zero Snap is planned as a separate module.)
- The playhead in the sample player keeps moving after the envelope finishes.

Not yet reported: clicks at the turns, several voices, p-locks, scenes, kit save and reload, power cycle.

Follow-up in rytm-emu (PP05 against stock 1.74, BD with AMP DEC 3, LOP ON, one hit): the sample voice's
play position (voice 0, sampled every 23M instructions) runs through the same values on both, looping
on long after the amp envelope has ended. With LOP OFF it stops at the sample's end. With PIPO it keeps
bouncing for as long as stock keeps an ON loop running. The voice keeps playing silently behind the amp
envelope on stock too, so this is stock behaviour, not something the module adds. (rytm-emu's mixed
output does not apply the AMP envelope to the sample layer, so audio could not show the silence.)
Which screen shows the playhead was not identified.

The switch icon is drawn from a two-frame sprite (17 x 9 pixels; OFF, ON) shared by twelve OFF/ON
parameters; stock clamps any value above 1 to the last frame, so PIPO drew as ON. npp1993 chose LOP to
look like BD CLASSIC's WAV switch: PP06 draws LOP, and only LOP, through the routine and frames stock
uses for that WAV (three positions), so the module carries no picture of its own.

## Code review (2026-10-27)

A `/code-review` of the branch raised ten points. What was done with each:

- **Fixed:** the PIPO text printed for value 2 on all eight parameters sharing the OFF/ON routine,
  and MIDI CC can put 2 in any of them. The gate now also checks that the call is for LOP's record
  (ctx 0x419a5760; a breakpoint on stock showed LOP's text called with exactly that).
- **Fixed:** the swap relied on stock rebuilding the effective STA and END every block. The handler
  now puts the values back itself when they still read as it wrote them a block ago.
- **Fixed:** the travel estimate took any move up to 512 frames, so a wrap could pass for travel.
  It now counts only moves in the direction the voice was going, keeps the last good value, and
  follows voices with LOP ON too, so a switch to PIPO mid-note starts with a known step.
- **Fixed:** a voice with no track kept its direction state; it is now cleared there too.
- **Fixed:** STA and END were converted to frames (four divisions) every block; the result is now
  cached per voice and recomputed only when STA, END or the sample change. Voices with LOP OFF
  return after one test.
- **Not a bug:** the trig mask is per voice. Stock's prefetch for voices 1-7 tests it with the
  voice index (`btst` with the loop counter) and looks the track up separately in the owner table.
- **Kept, documented:** turns land on block boundaries. A sample-exact turn would need the engine's
  own wrap, which the prefetch does a block ahead with a forward DMA; not cheap.
- **Kept out of the branch:** the release-notes file. `npm run check` rejects notes for a module the
  library does not list yet (Silent Pad+Trig has none either); it goes in with the library entry.
- **Done another way:** `module:doctor` does not reach `_submissions/`. Run on a temporary,
  uncommitted copy in `sdk/analog-rytm-mkii/modules/`, it passed manifest, category guide and
  module rules; its one red line, no pinned browser package, applies only to library modules.
  `node scripts/machine-modules.mjs --drafts` validates this folder.
- **Fixed:** the evidence hash now fingerprints exactly the source PP05 was built from.

## Sites

| Site | Length | What | Stock guard |
| --- | --- | --- | --- |
| 0x40121134 | 6 | `jsr` to voice 0's sample prefetch in the audio interrupt, after locks, scenes and LFOs are in the effective parameters; the gate runs the handler, then tail-jumps to stock | `2e4c18b1…7c8` |
| 0x400fb56c | 6 | the first two instructions of stock's shared OFF/ON value text; prints PIPO for 2 on LOP's record, else stock | `26c0ee91…7581` |
| 0x401c04f8 | 4 | data: LOP's maximum in stock's parameter table, 1.0 to 2.0 (8.8) | `6b1e73a0…0f93` |
| 0x400fedf4 | 6 | the first instruction of stock's shared OFF/ON switch icon; for LOP's record (ctx 0x419a5770, seen on a breakpoint on stock) it tail-calls BD CLASSIC WAV's three-position icon routine 0x400fef4a with the same arguments, else stock | `0528305f…fac3` |

Modwerk's linker links Pipo Loop beside Filter Hz, Loop Sync and Silent Pad+Trig together (PX04).
None of the four sites is patched by Silent Pad+Trig (0x400c772e, 0x400c7a3e, 0x400c7e90,
0x400c7950, 0x400ec184), Loop Sync (0x40121020) or Filter Hz. The OFF/ON text routine is shared by
parameter ids 34-37, 45 (LOP), 224, 248 and 463; the switch icon by twelve parameters (those and 73, 414, 415, 434).

## Load and memory

- Memory: 1,169 bytes in the shared mod area (897 of code and text, 272 of state), from PP06's package. The icon gate runs only when the screen draws a switch, not in the audio path.
- Load: static estimate from the compiled handler: about 15 instructions a block for a voice with LOP
  OFF or no track, about 70 for one looping with ON, about 105 for one in PIPO, plus four 32-bit
  divisions when a voice's STA, END or sample change. With the Rytm guide's emulator figure (0.023 %
  of a block per instruction a frame), about 0.1 % with nothing looping and about 0.6 % with all eight
  voices in PIPO. rytm-emu's breakpoints and profiler do not reach the audio interrupt's mod code, so
  it was not measured. Not measured on a unit.

## Not tested

- On a unit, beyond the PC03 report above.
- KIT SAVE and KIT RELOAD from the KIT menu, the sound browser, sound SysEx dumps and Overbridge.
- Eight voices in PIPO at once, LFO or velocity modulation of STA and END while in PIPO, and Loop
  Sync and Pipo Loop on the same track.
- PX04 booted.
