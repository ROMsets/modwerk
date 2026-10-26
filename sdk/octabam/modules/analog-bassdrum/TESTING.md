# Analog BD 0.1.6 — TYPE and FIXD switches

Date: 25 October 2026. Not tested on hardware or in `ot_emu`; software checks only.

- `bd909.asm` zq06 reads knob 11 at each trig. Nonzero: the click strength is `KPBASE` (0.96) and the noise LCG restarts from `SEED`. Zero: the 0.1.5 instructions run unchanged behind one test and branch.
- Offline `dsp_host` renders (`bd909.py`'s host, 2 s, five patches incl. ATK 0/TUNE 0 and ATK 127/SAT 60, four hits): with knob 11 at 0, every sample is bit-identical to 0.1.5's. Peak cost rises from 6523 to 6527 executed instructions per 16-sample block (the trig block); the same renders with FIXD ON peak at 6525.
- FIXD ON: the four hits of one render differ by at most 0.0006 of full scale over the first 3000 samples (filter histories carry over; about -64 dB), against 0.008-0.033 with FIXD OFF. The float model (`dsp909.Voice`) matches the DSP at -75.7 dB (OFF) and -75.2 dB (ON), as for 0.1.5.
- The 808 is untouched: it has no random source (`bd808.asm` and `dsp808.py` contain no noise generator).
- Both switches reuse the mechanism of stock's AMP SETUP SYNC control, read from the descriptor of the local OS: count 2, switch widget `0x40046f10`, no handler of its own. FIXD (slot 11, 909 only) also takes SYNC's own ON/OFF formatter; TYPE (slot 6, top left) has its own 808/909 formatter. Slot 6 is the model byte the engine browser edited, so saved projects keep their choice and the default (808) is unchanged. The page follows the model each UI tick as before.
- The engine browser is not opened any more: `ab_engine_open` opens the track's window and moves to its machine column, the two steps LEFT from the old browser took (stock's sample pool column never shows) and RIGHT on the Analog BD chooser row does nothing. The code stays in `control.c`, unreachable.
- The DSP renders above were repeated after the UI change: the engine is the same code (bit-identical results).
- Not tested: how either switch draws and turns on the unit or in `ot_emu`, that the model flips live, that the double-tap shows the machine list, and project save/reload. A `0.1.5` project with a stray nonzero byte in the old hidden slot 11 would read as FIXD ON (unlikely: the slot was never shown).

## 0.1.6 review fixes (26 October)

- The UI tick no longer reads the Part before the bank pointer is checked.
- TYPE and FIXD bytes outside 0/1 (a track set up with 0.1.5 holds ACCNT's value where FIXD now is) are put back in range on the selected track: TYPE to 909 (as the DSP played it), FIXD to OFF; both Part copies, the live controls and the stock editor's dirty flags.
- The module's own track-window handling is for card builds only now; over USB the base's LIST flag opens the window on the machine list. Card builds no longer show a pool chevron on the Analog BD row.
- The LIST behaviour is opt-in (FM Synth and the other machines are unchanged).
- `exact-render.json` holds 0.1.6's hashes; `verify_analog_bd_exact.py` passes.
- Not changed: the engine browser's code stays compiled in (unreachable); the private UI fixtures (`analog_bd_ui_fixture.cpp`) still name its symbols.

## 0.1.6 after the first MKII run (25 October)

Reported by the user on the RAM-booted base 7346a1f5 with Analog BD fbc09d07: the SETUP order, LPF direction and default, 4-letter labels, TYPE flipping live, save/reload of every value and SOURCE CLEAR (LPF back to OFF) work. HIGH did not move; the double tap flickered from the main page and showed an empty page when the window was switched to the Analog BD track from another track. Changes since, software-checked only:

- HIGH: the base's SRC SETUP editor hook (machine_registry.s, from POLY8's shared hooks) leaves encoder E read-only for every module machine, which is Analog BD's HIGH since the base took over the machine hooks (#585). A machine flag `SETUP_ALL` opts out; Analog BD sets it. Host test `test_machine_registry.c` covers the flag. The card build had the opposite rule (TYPE read-only); it now edits every slot.
- FIXD shows on both models. The 808 gets a per-hit variation for FIXD OFF: one LCG step per hit scales the trigger transient (PC, PS, DC) by 1 + 0.04 * white; FIXD ON is bit-identical to the 0.1.5 808 (`dsp_host`), and the float model matches the DSP at -100 dB. Cost: 37 DSP words (resident engine 1009 per core, 998 in 0.1.5) and about 20 instructions on a hit, none per sample.
- Double tap: the module opens the track's window about 200 ms (12 UI ticks) after the double tap, past its last key release, and puts it on the machine column in the same tick; a release reaching an open window would put stock's pool column back (the flicker the user saw, since an Analog BD track is FLEX underneath, where PICKUP's type gets the machine column). The UI tick also puts the window on the machine column when it switches to an Analog BD track, and leaves a pool column reached from another row alone. Second MKII run (base 1e022103): FIXD shows at once, T2 -> T1 shows the machine list, the direct double tap still flickered; the delayed open came after. Third run (base 89b8226d): no flicker, but the window came up a little later than on stock machines. Over USB the base now decides the column itself (`mw_chooser_column` at 0x400788c4): a module machine without a pool opens on the machine list, as PICKUP does, so Analog BD hands the double tap to the base and the window opens at once; the delayed open stays for card builds only. `ot_emu` (card build): T2's double tap then T1 shows the machine list; FLEX + RIGHT still opens FLEX's pool; T1's double tap shows the machine list.

## 0.1.6 SETUP layout, LPF direction, double tap

- SETUP is TYPE (slot 6), FIXD (7, 909 only), ACNT (8) above LOW (9), HIGH (10), LPF (11). LPF's tables are rotated so 0..126 run 500 Hz to 18 kHz (ORIG at 63) and 127, the default, is OFF (0.1.5's default was 18 kHz). The DSP reads the new slots; `dsp_host` renders of the 52 scenarios with each old setting moved to its new slot (LPF value v -> v-1, 0 -> 127) are identical to the previous build. FIXD OFF also matches 0.1.5 sample for sample under fast retriggers (every third block, varying offsets).
- Double tap: the track's window opens and, while it is open on Analog BD's row, the UI tick keeps it on the machine column, because stock's handling of the second TRACK press and its release returns to the (empty) pool column. In `ot_emu` on a card build of this source: the double tap shows SELECT MACHINE TYPE with ANALOG BD highlighted; LPF shows OFF at 127 and ORIG at 63; HIGH, LOW and LPF move with their encoders.
- SRC SETUP previews a machine's page while the track is still FLEX, whose LOOP byte sits in TYPE's slot; the preview now always uses the 808 page (the default after assignment), so the 808 page no longer shows FIXD after YES (`ot_emu`: before the fix it did until a control moved). In the preview the TYPE switch can still read 909 (FLEX's LOOP value) until YES.
- `tools/harness/bd909.py` is a pinned infrastructure import and stays byte-identical, so its patches keep the 0.1.5 slot order (in 0.1.6 they render with FIXD ON, ACNT 0 and LPF 500 Hz). The DSP and the float model read the same slots, so the gate still compares like with like.
- `exact-render.json` holds 0.1.6's hashes now (all 16 audio and state hashes; `verify_analog_bd_exact.py` passes). The pins it replaced were already stale on `main`; 0.1.6's own changes to them are the 808's per-hit variation (FIXD OFF in the default, minimum and automation cases) and the new slot meanings (the `maximum` case now has FIXD ON and LPF OFF).
- Not tested on the unit yet.

## 0.1.6 DSP efficiency (bit-identical)

Both engines are rescheduled without changing a result: the desk skips its three SAT table interpolations once SAT has settled (SSAT equals SAT << 16, so drive, make-up and coupling already hold those entries), immediate coefficient and table loads ride in the parallel-move slot of the ALU instruction before them, `tfr` with a parallel immediate replaces a register copy plus a load, and the VCO phase, the pulse HP and the biquad histories stay in registers instead of being reloaded.

- `dsp_host`, 52 scenarios (26 per engine, 1.2 s each: defaults, every control at both extremes, FIXD ON, SAT steps, ramps and random jitter, random automation of every control, retriggers at random offsets): every output sample identical to the previous build. 909 peak 6534 -> 5999 and mean 6488 -> 5519 executed instructions per 16-sample block; 808 peak 4260 -> 4034, mean 3979 -> 3345.
- `verify_analog_bd_exact.py`'s cases (all trigger offsets, rapid retriggers, long tails, control extremes, random block-rate automation; 16 audio and state hashes): identical before and after. Its pinned 808 `maximum` state hash was already stale on `main`.
- Resident engine: 972 P words per core (998 in 0.1.5). The declared DSP price (411 per voice) is unchanged; it comes from 0.1.5's full-chain emulator measurement, which was not repeated, so it now over-states the cost.
- Not measured: chip cycles (parallel long immediates are two-word instructions) and full-chain loads.

# Analog BD 0.1.5 — FLEX restore regression

Date: 9 October 2026. Tester and approving owner: repeat98 (Jannik Aßfalg).

The exact private candidate offered for the hardware check was **AB015FIX01**, module 0.1.5-experimental. After the request to switch Analog BD to FLEX without SRC + PLAY and power-cycle, the owner replied:

> flex persists. merge and update

This is a reported physical FLEX reboot-persistence result, limited to that statement. Model, duration, audio/transport behavior, number of instances, Part/project save/reload results and maximum load were not supplied. It does not establish the Output Matrix sequencer-halt fix. The owner directed this exact release after being informed that 0.1.4 approval did not cover it and broader checks remained missing. The separate sdk/analog-bassdrum-build-approval.json records the new version/source approval and its remaining qualification limits.

The candidate machine source was copied byte-for-byte into the public module. Native source SHA-256: `e741e4876ea3ae44eb54ddc93ff3b5bf5f65ebb3e9320cc5fd1f602f7925966d`. Private MAIN OS SHA-256: `137580f326b6adc3b0d58f2183d530507f1b370c48911b38d5d0cf94d9e96898`. Saved private upgrade SHA-256: `5737e32c4823fffa3236469e82425b3eb53ba75da1d8d0a690b824eaefeb63e2` (568,928 bytes); its decoded MAIN OS equals the native image.

## Software regression

The compiled ColdFire hook restores six playback and six SETUP bytes from the owned stock defaults at runtime before clearing a valid AB/1 marker. All four callers, eight tracks and four Parts pass 512 marked/unsigned/malformed cases; 32 Analog BD reselection cases and all five other machine destination rows pass. Both volatile and battery-backed Part copies are checked, along with unchanged unrelated bytes and register/stack behavior. The 0.1.4 writer fails the restore assertion.

Current-source DSP-enabled ot_emu receives actual panel assignment and switching. A separate process then restarts with only the captured battery RAM and saved card, `--cs1-in --no-post`; no LOAD PROJECT is posted. 0.1.4 switch rejects the bank and restores all eight STATIC machines. The candidate switch restores all eight FLEX machines. Leaving 0.1.4 assigned Analog BD restores its marked track as a control. Sanitized records and reproducible gates are in sdk/drafts/analog-bassdrum-flex-restore/evidence/chooser.json and evidence/reboot.json, verify.py and reboot.py. These are emulator results, distinct from the owner report.

Both cores' DSP code and complete DSP uploads are byte-identical to 0.1.4. The ColdFire runtime grows from 14,016 to 14,052 bytes (36 bytes). No new DSP state allocation is introduced; the copy occurs during machine assignment, not in the audio loop. Historical DSP instruction measurements below remain historical, not new chip-cycle measurements. Complete memory/stack lifetime bounds, maximum FX load and broader current-build hardware checks remain unverified under the exact owner-approved experimental release.

## 0.1.5 common-builder comparison

116 selections: 46 native module-owned OS/DSP images match, 70 matching native/browser refusals, zero byte mismatches. Four of the 46 native images fit the smaller logger-free runtime but are refused by the browser’s mandatory logger boot-memory guard; these are explicitly recorded as additional browser platform refusals, leaving 42 buildable matched selections. No guard is disabled and no firmware is generated for those crowded layouts. The standalone native image remains identical to tested AB015FIX01; the downloadable image additionally carries the mandatory core logger and is checked independently by saved-download identity.

## Historical 0.1.4 reference qualification

# Analog BD 0.1.4 — 909 direct-out reference update

Current release uses the owner-auditioned **AB014REF05** DSP/fit source. The owner replied “it's ok, let's release it”, then explicitly confirmed “Yes—waive those limits and release” for fresh physical hardware/persistence, worst-case chip timing and complete memory bounds. This is a new exact-source approval; the 0.1.3 physical report below remains historical. Current hardware status is **untested**, not reported or verified.

The 909 body phase/asymmetry, envelope/DC and both Tune endpoints are fitted to Skee Mask's 70 BPM direct-out recordings. Attack is fitted at LPF bypass; the real 909 main-out LPF was not in the recording path. The reference instrument has a decay modification; its complete modified range is not modeled. Intermediate recorded knob positions remain unknown, and endpoint repeats may still contain knob motion.

Current evidence lives in `evidence/909-reference-*.json`; the full analysis/methods are in [the reference testing report](../../../drafts/analog-bassdrum-909-reference/TESTING.md). The exact pre-promotion native gates were run from commit `d96ae7f` against the approved 0.1.3 SDK snapshot, then these three DSP/fit files were copied byte-for-byte into the release folder. Their individual hashes and the tested private MAIN OS are recorded in port/private-build evidence. Tests do not play captured samples.

- Native min/max, SAT 0/64/127, moving controls, rapid retriggers and all 16 trigger offsets pass. All 128 values of ten audible controls are fraction-bounded and pass model comparison. Quiet all-minimum cases pass explicit absolute quantization limits rather than relative error.
- Four interleaved distinct voices/core pass control/reset isolation; both combined code origins match standalone audio. Tested 808 audio and instruction counts remain bit-identical to 0.1.3.
- Actual complete-image source/control/stock AMP output passes at TUNE/ATK 0/0 and 0/127 on core A and 127/0 on core B, with 808 on the other core. Eight complete repeated high-Attack hits were captured. Source/model error is −72.1 to −72.7 dB; AMP/source residual −118.1 to −118.6 dB. The saved private upgrade decodes exactly to the tested image.
- Matched both-core 2048-block instruction runs give 909 mean 6524.07/peak 6563 per block versus 6141.77/6176 for 0.1.3: +6.22% mean and +6.27% maximum matched-block increase. 808 mean 4268.47/peak 4294 remains unchanged. These are executed instructions, not modeled chip cycles or hardware timing.
- Code occupies 997 P words/core plus the separate 35-word stock helper, within the existing reservation. X upload remains 3776 words/core, four 64-word voices/core; Y and ColdFire allocation are unchanged. Complete lifetime/stack bounds remain unmeasured.
- The original treble analysis clipped the reference partway into the attack, creating an artificial discontinuity. Corrected full-onset measurements include 512 samples of natural pre-roll. Current upper-band deficits and neighboring-shape mismatch remain explicit; the private listening clips always contained full hits.
- Eight native equal-control hits have 2.23% peak spread, down from 3.42%. Seven carried-state firmware-emulator hits have 1.81% tail-normalized peak spread. After fitted level/alignment removal, reference final-four shape spread is 3.12%/3.01% over 0–10/10–30 ms, versus 0.86%/0.24% in the emulator. Faithful individual-hit reproduction is not established.
- Current-source physical audio, Part/project/reboot persistence, demanding FX load, worst-case chip timing and complete memory bounds remain untested/unmeasured. The exact owner exception does not convert them to passes. High ACCNT/LOW can still reach the existing output limiter.

Fresh DSP-enabled UI capture passes on AB014REF05: source assignment, engine browser and 808/909 pages were captured with the disposable loaded project, then all four PNGs were opened and reviewed. The 808 shared page reads TUNE for its third encoder; its underlying TONE behavior is unchanged. Current media metadata documents the displayed label. The isolated pinned source toolchain compiled all current source-only packages without firmware. Development import validates their complete source inventory and version pins. Native/browser parity passes all 112 configurations: 36 matching builds outside existing platform writes, 76 matching refusals, zero mismatches. Node 24.21.0 `npm run check -- --base origin/main` passes after integration with main `d1e38bc`: 1,402 app tests in 200 files, 86 SDK tests, catalogue/licence checks, lint, types and the production bundle. Existing lint/bundle warnings remain. The broader 136-profile interaction matrix is regenerated from the current source; its native/bootstrap verification and clean-commit package reproduction are completed before publication.

## Historical 0.1.3 qualification — retained without relabelling

# Analog BD modulation fix — testing

Release: `0.1.3-experimental`; approved base: `0.1.2-experimental`.
Owner report, 8 October 2026: modulating TDEP and SAT produces audible zipping.
The report did not identify the device, image hash or modulation settings.
The later owner hardware result and release approval are recorded below.

## Native evidence — 8 October 2026

The isolated Linux DSP interpreter assembled and disassembled the exact candidate.
[evidence/native.json](evidence/native.json) binds source, base, tool and test
identities to the results. No stock firmware was used in these tests.

- Ten constant renders (808/909, SAT 0/64/127 and all-controls minimum/maximum) are bit-identical to the approved
  sources: 13,216 stereo samples each, all trigger offsets and rapid retriggers.
- Stepped TDEP, SAT and combined automation match the independent sample-wise
  reference with residuals from −92.39 to −106.31 dB. Actual native state checks
  verify upward/downward slews and continuity through retriggers.
- Full-range first-sample parameter-change deltas fall by 36.07–36.26 dB for
  909 TDEP, 28.02–58.35 dB for 909 SAT and 39.08–43.49 dB for 808 SAT. The probes
  compare a changed control to a held control with the same preceding state;
  these are deterministic jump measurements, not listening scores or a guarantee
  about every patch/modulation rate.
- The compact combined image renders exactly like the standalone engines at
  both native P origins (A `0x1252`, B `0x1012`). It uses 946 P words per core
  against a 1,028-word region, preserving the separate stock-helper reservation.
- Four interleaved instances per core placement use distinct patches and two
  808s/two 909s. Every audio stream is bit-identical to that voice alone. Editing
  their controls and resetting one 909 does not change the other voices. The host
  uses the native 64-word voice stride and stores knobs at voice +48, exercising
  the reused scratch slots. These are DSP-core placement/instance tests, not
  full-machine or physical-device tests.

The initial approach slewed SAT's three coefficients independently. The 808
full-range jump probe exposed gain overshoot and insufficient step reduction.
The delivered candidate slews SAT position before table interpolation; all probes
above then pass. The macOS execution sandbox failed before Python started, so
only the isolated Linux results are claimed.

## Resource accounting and limits

Combined peak executed instructions in the focused modulation/retrigger workload
are 6,176 per 16-frame 909 block (386/sample) and 4,294 per 808 block (268.375/sample).
These are measured executed instructions, **not worst-case chip cycles**. They
exceed the approved standalone harness's old instruction-growth guards, so that
old timing evidence must not be used to release this update. Full-chain budgets
and maximum audio/FX load remain untested.

P allocation: 946 words / 2,838 packed bytes per core; the 35-word stock helper
stays in its separate reservation. The existing X upload region remains
`[0x2840,0x3700)`: 3,776 words / 11,328 packed bytes per core, including tables,
voice blocks and alignment gaps. Both engine banks keep four 64-word blocks per
core, 512 words / 1,536 packed bytes in total. No X, Y or ColdFire allocation is
added. Per voice, SAT history uses previously unused X word +61. The 909's depth
histories reuse +21/+22; temporary body/pulse scratch moves to consumed MODEL
and unused-control slots +54/+59. Controls are recopied before each block. Init
clears PAD, so first-use seeding also resets the SAT history without clearing
another voice. ColdFire assignment, transport and saved control layouts are
byte-identical to the approved base (bound in [evidence/native.json](evidence/native.json)).

Relative resource estimates: ColdFire demand unchanged; DSP remains high per
active voice because synthesis plus desk now also performs per-sample smoothing
and table interpolation; memory demand is unchanged. These estimates do not
replace the missing full qualification accounting/timing.

## Reproduction

Use an existing reviewed native-toolchain image with Python, a C++ compiler,
DSP assembler/disassembler and emulator static libraries at `/opt/toolchain`.
The local run used `octamod-tapehead-qualification-tools:local`, image ID
`3a5861370c0f`. The exact assembler/disassembler hashes are in the evidence.
Keep the source mount read-only, disable network, drop capabilities, and allow
writes only to a new temporary workspace and container tmpfs:

```sh
docker run --rm --network none --read-only --cap-drop ALL \
  --security-opt no-new-privileges --pids-limit 128 --memory 2g --cpus 2 \
  --shm-size 256m \
  --mount type=bind,source=<repository>,target=/source,readonly \
  --mount type=bind,source=<new-private-workspace>,target=/work \
  --tmpfs /tmp -w /work -e PYTHONDONTWRITEBYTECODE=1 \
  octamod-tapehead-qualification-tools:local \
  python3 /source/sdk/drafts/analog-bassdrum/qualification/run.py --output /work
```

Use the original pinned 0.1.2 SDK snapshot when running these historical development scripts; the promoted module is already overlaid. The runner verifies input identities before creating the disposable SDK overlay,
compiles the two test hosts, then runs baseline, static/modulation, jump and
multiple-instance gates. Only sanitized `evidence.json` may be retained publicly;
compiled payloads, raw audio/state, firmware, projects and cards remain private.

## Firmware persistence and actual LCD — 8 October 2026

[evidence/persistence.json](evidence/persistence.json) binds the private candidate
image (`87e56b38…`) and emulator to the tested source. The existing disposable
fixture was copied, not edited in place. T1 runs 808 and T5 runs 909, with different
patches and both stock FX slots populated. Part save/reload restores a saved
baseline after unsaved edits. Panel-driven PROJECT SAVE/RELOAD restores both
tracks after later edits. A fresh explicit project load also matches.

A new emulator process retaining only the saved card and 1 MiB battery RAM
restores both tracks without posting LOAD PROJECT, including seven seconds with
frame interrupts. Machine/FX assignments, source/engine/sample settings and
source/FX main/setup controls match throughout. Editing T1 preserves T5 and
editing FX2 preserves FX1. These are **control-state passes**, separate from the
DSP instance/audio tests above. No physical device was rebooted.

The initial DSP-enabled probe exited with SIGBUS (exit −7), before usable
audio or capture readiness. The actual controller LCD captures therefore used
the working stand-in DSP path. A private copy of `scripts/capture-module-ui.py`
removed its forced `--dsp` argument and set a 20-second project-load budget; the
source LCD renderer and actual panel actions were unchanged. Both tool hashes,
exact actions, source/image/emulator identities and visually reviewed PNG hashes
are retained in [media/capture.json](media/capture.json). Those historical captures
remain labelled stand-in DSP. An initial plan left a DISARM popup on two captures;
those were rejected and recaptured after it expired.

## Full-emulator SIGBUS diagnosis and fix — 8 October 2026

[evidence/emulator-shm.json](evidence/emulator-shm.json) records the regression and
recovery on the unchanged candidate image (`87e56b38…`) and unchanged emulator.
Docker's default `/dev/shm` allowance is 64 MiB. The DSP library reserves 52 MiB
of shared-memory backing per core (104 MiB for the pair), including the
invalid-address backing block. Mapping succeeds before storage is touched;
constructing the second DSP core exhausts that pool and raises SIGBUS.

An identical `--dsp --max 1000` probe exits −7 with `--shm-size 64m`; with
`--shm-size 256m` it constructs both cores and stops normally at the intentional
instruction budget (exit 1, BUDGET). The existing isolation settings and 2 GiB
overall memory limit stay in place. No firmware or emulator source change was
needed. The capture tool now checks for at least 128 MiB free in `/dev/shm` on
Linux, reports the `--shm-size 256m` setting and refuses before creating its output
directory. Both the real 64 MiB rejection and 256 MiB acceptance passed.

With 256 MiB, the full candidate boots both DSP cores, passes the RTOS gate and
completes LOAD PROJECT with distinct T1 808 and T5 909 assignments and both FX
slots populated. The initial 64-frame playback probe reaches its frame target
and captures 623,039 eight-channel frames, but all samples are zero: it did not
reach a trigger. That probe establishes crash recovery and project loading,
not usable audio. A separate 1,024-frame probe with `--internal-clock` reached
its 180-second wall-clock cap and was killed; no usable-audio pass is claimed.
The follow-up ran Node 24 `npm run check -- --base origin/main`: all 185 test
files / 1,249 tests and lint/build/SDK/catalog checks passed.

## Open acceptance paths

- Usable full-chain audio after Part/project reload and battery-only restart:
  **owner-reported physical MKII pass**, confirmed below. The emulator audio path
  remains unverified; emulator control-state passes are separate.
- Full machine reset/model-replacement isolation and maximum audio/FX load:
  **untested**. Native voice reset/isolation tests do not replace these checks.
- Worst-case chip cycles and complete final-image memory bounds: **unmeasured**,
  accepted for this exact owner-approved experimental update. Native/browser
  composition and release packaging are checked separately below. The
  owner-attributed MKII operation/reload/reboot/audio report is recorded below.

No physical device was flashed by the agent. The owner tested the private build.
The private candidate image, cards,
raw audio/LCD/memory traces and logs are not published. The owner requested release of this exact tested update. The separate source-bound
owner approval preserves the named incomplete measurements as explicit limits. The existing approved thumbnail is retained;
this update changes modulation behavior without changing the control design.

## Owner functional hardware report and release approval — 8 October 2026

Jannik Aßfalg (`repeat98`) received `OCTATRACK_ANALOG_BD_0.1.3_TEST.bin`,
label `AB013TEST`, built from commit `207964506864dec8a71ca0bc35ca19aca46a6f48`.
Complete update SHA-256:
`48217f8b1bf2b9284fc5831a79b94f020c70f1dafc94f6a72ff6857765a83a5a`.
Decoded MAIN OS SHA-256:
`87e56b38fe45a5b10fda2129193f67946b078f4d84b07a0cb1afbc2ef0ebaa54`.
The file passed native/browser full-update byte parity, checksum and decoded
MAIN identity checks. Firmware remains local.

The owner's exact statement was: **“works great and stable, let's release”**.
This is a credited functional hardware report and release authorization for
`0.1.3-experimental`, not a measured stress or worst-case timing result. The owner then answered **“MK2 yes all”** to the question asking for the model
and whether distinct 808/909 instances, Part/project reload and normal power-off/on
all kept their settings and audible output. This records an MKII physical
functional/persistence pass, separately from the emulator evidence. Duration,
maximum instance/FX load and worst-case chip timing were not reported.

The separate `sdk/analog-bassdrum-build-approval.json` binds the release approval
to this version and final native-source fingerprint. It preserves the existing
owner-approved update path for worst-case chip cycles and complete memory bounds. These exceptions do not grant later versions
a pass; source integrity, native/browser composition, packaging, licences, UI
provenance and documentation remain required.

## Release composition and packaging — 8 October 2026

The promotion was rebased onto approved main `f7ba0d1`, including Mini Verb 0.2.0.
A clean, tracked-source-only build at `fdb94e7ab46cb9240962456594819650ec4bc1fa`
compiled the published 0.1.3 packages without stock firmware. Toolchain image:
`sha256:6711f0abb3c30dcfda4e9a8a812555f8bbceb37a0fc988918b5d624b603d4a6a`.
Source tree SHA-256:
`dabaf66190efa66d07ddd7a5cc4eed7baf7efa4980887e093d733edb3ac4e053`.
Every other generated code package is unchanged from approved main; the requested
package changes only Analog BD's engine data and associated hashes/word counts.

`npm run module:verify -- analog-bassdrum --os <owner-stock> --jobs 1
--image modwerk-source-tools:continuation` passed 112 selections: 36 native/browser
matching builds and 76 matching refusals, zero mismatches. The record is
`sdk/native-comparisons/analog-bassdrum.json`. Changed stock input is refused.
Use one worker for this native builder: its DSP assembly output paths are shared;
a two-worker probe raced those paths and failed the decode preflight.

The native Analog BD suite then refreshed all 136 DSP/utility profiles, with
both stock-FX2 menu choices: 130 builds and six expected refusals. The source-bound
fingerprints are in `src/engine/assets/analog-bd-composition-proofs.json`.
`node scripts/verify-analog-bd-native.mjs <owner-stock> <proofs>` passed all 130
module-owned MAIN comparisons and independent GNU bootloader byte comparisons,
six matching refusals and five complete browser firmware round trips. Stock input
is unchanged and no platform/logger writes overlap module-owned writes.

Four full native update packages match browser ELEK/ELUP encoding byte-for-byte:
Analog BD alone and Analog BD + Tape Echo + all five utilities, each with stock
FX2 kept and compact. Every decoded MAIN, container tail and checksum passes.
Compression is sampled here; the 136-case matrix checks composition/bootloader
bytes for every accepted profile. Firmware and native packing inputs stay private.

The standalone native MAIN is
`e5639f9f47feaaaf74d202c411409d42a021017d0067e3c5aa4f2493798fd8d3`.
Only 50 bytes differ from the owner's test MAIN: the shared builder clones the
identical stock FX1 chooser table at `0x400d6b20` and changes its three references
at `0x40037990`, `0x40052706` and `0x40059bd2`. Restoring those four chooser writes
reproduces the owner's entire MAIN hash exactly, including the appended payloads.
The test image retains the original stock FX1 references; both menus are identical.
The public worker also adds its already-approved logger/startup infrastructure.
The standalone public-worker composition, label `ELEKLOADER`, round-trips to MAIN
`80dfdd8f938daaf86f87e1332cec93a735a9c3ba00997ce70a20b9886b2fdeac`;
full update is 575,496 bytes, SHA-256
`c871fc7424404985ca2c4c1a2ee3be1fa5c68e47494c6abf66ef4e3d479dca33`.
These are software checks; the credited physical report remains bound to the
actual `AB013TEST` image and is not relabelled as a physical test of every composition.
