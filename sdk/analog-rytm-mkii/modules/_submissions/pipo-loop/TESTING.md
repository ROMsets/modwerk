# Pipo Loop — test report

Everything below was observed in rytm-emu on the named build. Nothing has run on a unit yet.

- Source (0.1.0-experimental; PP03 was built from it before `build.json` gained its `claims`, which the linker does not read): `build.json` `209a491b…1759`, `src/audio.s` `9a41a2ed…b787`,
  `src/pipo.c` `c08b476a…cfe9`. Fingerprint (`sha256` of `sha256sum build.json src/audio.s src/pipo.c`):
  `840fefdb…edc8` (`evidence.sourceSha256`).
- Stock OS 1.74: `.syx` `cb0f99d6…e123`, the owner's file.
- Core: elekloader core-ar2 0.3 (`core-ar2-0.3.elemod` `6b97dfb3…3e58`, as vendored).
- Build PP03 (core-ar2 0.3 + Pipo Loop 0.1.0-experimental, Modwerk's linker, `--version PP03`):
  `.syx` `7c40259949fbc2b28a00f510afe81f4afc6a6a26ec4a15239aa4eed6d0130554`, main OS `a1829db9…c4dc83`.
- Build PC01 (core-ar2 0.3 + Silent Pad+Trig 0.5.1 + Pipo Loop 0.1.0, `--version PC01`):
  `.syx` `2f6d90e863573d1757463e9d1378635a77e5076b2c57f07d2ceb028ed8df54e0`. The same pipeline builds
  Silent Pad+Trig alone byte for byte as its PS08 (`ff7c4cf3…43d4`).
- Emulator: rytm-emu `c5732f0`, headless `dtboot`, JIT on. A card image holds one generated sample,
  RAMP (24,000 frames, a straight rise from -26,000 to +26,000), loaded to the project and set on
  BD with LOP ON on the SAMPLE page (the setup in [media/capture.json](media/capture.json)). Audio is
  dtboot's `--wav` with `--every 20M` (longer report intervals keep only about a second of audio).
  Effective LOP is read from the track's effective parameter word (`DTBOOT_PEEK`).

## Emulator, 0.1.0 (2026-10-27)

| Check | Stock 1.74 | PP03 |
| --- | --- | --- |
| LOP turned clockwise from ON | stays ON | reaches PIPO; the value text reads PIPO, the switch icon draws ON |
| LOP turned back | ON, OFF | PIPO, ON, OFF |
| Ramp, LOP ON, one BD hit | sawtooth, period 24,000 frames, a 12,044 step at each wrap | identical |
| Ramp, LOP PIPO, one BD hit | (no PIPO) | triangle; turns every 23,935-23,937 frames; largest sample-to-sample step 5 (of 32,767) |
| PIPO about 2 semitones up / down | — | triangle, segments 21,311-21,313 / 26,880 frames, largest step 7 / 5 |
| PIPO at TUN +24 | — | triangle, segments 5,952 frames (6,000 ideal), largest step 5 |
| P-lock: GRID RECORDING, trigs on steps 1 and 9, LOP locked to PIPO on step 1, PLAY | — | lock shows PIPO; effective LOP alternates PIPO (step 1) and ON (step 9); triangle from step 1, saw from step 9 |
| Retrig while playing backward (step 9 above) | — | restarts forward from STA |
| Scene: SCENE held, BD pad held, LOP turned | — | footer SCENE: 1, LOP locked to PIPO; toggling scene 1 in SCENE mode switches effective LOP between ON and PIPO |
| Two voices: BD and SD both RAMP, LOP PIPO, hit together | — | both channels ping-pong (BD clean; SD's channel also carries its analog snare) |
| MIDI CC 30 on channel 1: 0, 32, 63, 64, 85, 86, 127, 0, 1, 2 | LOP = the CC value, unclamped | identical (so CC 2 gives PIPO) |
| PC01: ramp with LOP ON / PIPO | — | saw / triangle, as PP03 |

The turn needs one block of lookahead: with none (tested by setting it to 0), stock wraps the voice
to STA first and the saw comes back. The prototype looked two blocks ahead (turns 23,872 frames
apart); one block halves the early turn and stays click-free at every pitch above.

### Saving and stock compatibility

- PP03 with BD on LOP PIPO, power-off at 15,500M (`--power-off`; the firmware's handler saved the
  working state, 4,096 sectors): the card then cold-booted on **stock 1.74** with effective LOP 2
  (no clamp) and a BD hit played a **normal forward loop** (sawtooth, period 24,000, as ON).
- The same card cold-booted on PP03: LOP PIPO came back and BD ping-ponged.

### Captures

`media/lop-pipo.png`, `media/plock-pipo.png` and `media/scene-pipo.png`: PP03 frames, plans in
[media/capture.json](media/capture.json).

## Sites

| Site | Length | What | Stock guard |
| --- | --- | --- | --- |
| 0x40121134 | 6 | `jsr` to voice 0's sample prefetch in the audio interrupt, after locks, scenes and LFOs are in the effective parameters; the gate runs the handler, then tail-jumps to stock | `2e4c18b1…7c8` |
| 0x400fb56c | 6 | the first two instructions of stock's shared OFF/ON value text; prints PIPO for 2, else stock | `26c0ee91…7581` |
| 0x401c04f8 | 4 | data: LOP's maximum in stock's parameter table, 1.0 to 2.0 (8.8) | `6b1e73a0…0f93` |

Modwerk's linker links Pipo Loop beside Filter Hz, Loop Sync and Silent Pad+Trig together (build
PX01 `37073504…6f42`; linked only, not booted). None of the three sites is patched by Silent Pad+Trig (0x400c772e, 0x400c7a3e, 0x400c7e90, 0x400c7950, 0x400ec184),
Loop Sync (0x40121020) or Filter Hz. The OFF/ON text routine is shared by parameter ids 34-37, 45
(LOP), 224, 248 and 463; only LOP's range reaches 2.

## Load and memory

- Memory: 764 bytes in the shared mod area (code, the PIPO text and 48 bytes of state), from PP03's link map.
- Load: static estimate from the compiled handler: about 27 instructions a block for a voice not in
  PIPO, about 115 for one in PIPO (four 32-bit divisions). With the Rytm guide's emulator figure
  (0.023 % of a block per instruction a frame), about 0.2 % with no voice in PIPO and about 0.7 % with
  all eight. rytm-emu's breakpoints and profiler do not reach the audio interrupt's mod code, so it
  was not measured. Not measured on a unit.

## Not tested

- Anything on a unit.
- `npm run module:doctor -- pipo-loop` does not reach `_submissions/` folders; `node scripts/machine-modules.mjs --drafts` validates this folder.
- KIT SAVE and KIT RELOAD from the KIT menu, the sound browser, sound SysEx dumps and Overbridge.
- Eight voices in PIPO at once, LFO or velocity modulation of STA and END while in PIPO, and Loop
  Sync and Pipo Loop on the same track.
