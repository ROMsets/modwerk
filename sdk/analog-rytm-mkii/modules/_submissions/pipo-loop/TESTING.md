# Pipo Loop — test report

Everything below was observed in rytm-emu on the named build, except the Hardware section, which records what npp1993 reported from a unit.

- Source (0.1.0-experimental): `build.json` `a6f8f799…82cd`, `src/audio.s` `b7ef777e…27f7`, `src/pipo.c` `5f1a2efd…9dbf`,
  `src/snap.c` `16eae916…e1bf`. Fingerprint (`sha256` of `sha256sum build.json src/audio.s src/pipo.c src/snap.c`):
  `f5d3bbb8…24b8` (`evidence.sourceSha256`).
- Stock OS 1.74: `.syx` `cb0f99d6…e123`, the owner's file.
- Core: elekloader core-ar2 0.3 (`core-ar2-0.3.elemod` `6b97dfb3…3e58`, as vendored).
- Build PP10 (core-ar2 0.3 + Pipo Loop 0.1.0-experimental, `--version PP10`), from that source: `.syx`
  `1107d5ec0e18e6e710044c8b7d5a3066cc24d9d1f3dd1a0cbccd32ec965bd62f`.
- Build PC08 (core-ar2 0.3 + Silent Pad+Trig 0.5.1 + Pipo Loop 0.1.0, `--version PC08`): `.syx`
  `e706dcf3af4544f340b373001cadd749d14a5bbaff6fccc33d1e6f5170cf85b8`.
- Diagnostic PD02 (PC08's code plus an on-screen readout, branch `diag/pipo-snap`; not for release): `.syx`
  `d2f3c51f0c495ed821ee2bcda7b4e6f068a5b8f75d69e93d30925c8b52e47a5e`.
- Build PX07 (with Filter Hz, Loop Sync and Silent Pad+Trig): `.syx` `7dc637fc…`. Linked only, not booted.
- Earlier builds: PP03 and PC01 (before the code review), PP05 and PC03 (after it), PP06 and PC04 (the
  three-position switch), PP07 and PC05 (zero snap on the sound), PP08 and PC06 (locks), PP09 and PC07 (exact turns);
  each passed the battery below. PP10 gave the same results as PP09 on every row of it.
- Emulator: rytm-emu `c5732f0`, headless `dtboot`, JIT on. A card image holds one generated sample,
  RAMP (24,000 frames, a straight rise from -26,000 to +26,000), loaded to the project and set on
  BD with LOP ON on the SAMPLE page (the setup in [media/capture.json](media/capture.json)). Audio is
  dtboot's `--wav` with `--every 20M` (longer report intervals keep only about a second of audio).
  Effective LOP is read from the track's effective parameter word (`DTBOOT_PEEK`).

## Emulator, 0.1.0 (2026-10-27)

Every row was run on PP09, except the MIDI row (PP03) and the switch rows (PP06, the same drawing code);
PP03, PP05, PP06 and PP07 gave the same results.

| Check | Stock 1.74 | PP09 |
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
| PC07: ramp with LOP ON / PIPO | — | saw / triangle, as PP09 |

The turn needs one block of lookahead: with none (tested by setting it to 0), stock wraps the voice
to STA first and the saw comes back. The prototype looked two blocks ahead (turns 23,872 frames
apart); one block halves the early turn and stays click-free at every pitch above.

### Saving and stock compatibility

- PP09 with BD on LOP PIPO, power-off (`--power-off`; the firmware's handler saved the working state):
  the card then cold-booted on **stock 1.74** with effective LOP 2 (no clamp) and a BD hit played a
  **normal forward loop** (sawtooth, period 24,000, as ON).
- The same card cold-booted on PP09: LOP PIPO came back and BD ping-ponged.

### Captures

`media/lop-off.png`, `media/lop-on.png`, `media/lop-pipo.png`, `media/plock-pipo.png` and
`media/scene-pipo.png`: PP06 frames, plans in [media/capture.json](media/capture.json).

## Playhead on the SMPL WAVEFORM view (2026-10-27)

Pressing SMPL on the SAMPLE page opens SMPL WAVEFORM, which draws a playhead line. Stock 1.74's rules,
from its code: the page's tick takes the selected track's voice (track-to-voice table) and shows the line
only while that voice still belongs to the track and the voice's active flag (record + 40) is set; the
line's x is the voice's play position over the sample's length, across the whole sample (not STA to END),
refreshed on the page's tick. The active flag stays set while a looping voice runs, after the AMP envelope
has ended too, and clears when a one-shot voice (LOP OFF) reaches its end.

rytm-emu, stock 1.74 against PP06, ramp on BD with AMP DEC 3, frames 0.1 to 4 s after one hit: stock LOP ON
and PP06 LOP ON move the line identically (the same positions) and keep it moving after the envelope; LOP
OFF hides it at the sample's end; PP06 PIPO moves it forward to END, then back to STA, and so on (the
voice's direction byte, + 42, is 1 on the way back, as for stock's reverse play). Pipo Loop changes
nothing here: the line follows the voice's real position, so PIPO reads as expected.

## Zero snap: FUNC + STA / END (2026-10-27)

Test sample ZSIN (24,000 frames): a 141.7-frame sine of amplitude 20,000 with a phase offset (no crossing
at frame 0), silent from 9,000 to 11,000 and at a DC of 3 from 15,000 to 16,000; 296 crossings, computed
offline with the module's rule. ZS01 (the same main OS as PP07), SAMPLE page, SMP 2:

| Check | Result |
| --- | --- |
| FUNC + STA, 16 detents forward from 0 | STA stepped 54, 124, 195, 266, 337, 408, 479, 550 (frames): the first eight crossings, in order (the other detents fell to stock's encoder hysteresis) |
| then 8 detents back | 479, 408, 337 |
| FUNC + END back from the end | 23,930, 23,859, 23,788, 23,717: consecutive crossings |
| one detent at a time, settled for 1 s | the stored value's frame is the crossing exactly (16,278; 16,207) |
| FUNC + STA back from 16,335 across the DC run | 16,278, 16,207, 16,137 (glide values 16,138 and 16,136 seen), 16,066, then 15,000 (the edge of the DC run, next to -3,441), then 14,932, 14,861, ...: the near-zero DC is passed over |
| plain turn on STA | fine steps, as stock |
| FUNC + turn on STA with a trig held in GRID RECORDING | the lock moves in stock's fine steps (0, 0.12): lock editing takes stock's other branch, which the site does not touch |

Every value in between is stock's parameter glide on the effective value; the stored values sit on the
crossings. STA and END are 1/30,720 of the sample, so for samples up to 30,720 frames each crossing has an
exact value; past that the nearest value is taken, at most frames / 61,440 frames off: 0.8 frame at 1 s,
3.9 at 5 s, 7.8 at 10 s, 23 at 30 s, 47 at 60 s (48 kHz). Not tested: a sample over 30,720 frames, a
sample with no crossing at all (the code leaves the turn to stock), STA and END meeting.

### Next crossing, not the other half of the same one; locks (2026-10-27)

PP07 had a flaw: a point on a crossing sits on the sample of the pair nearer zero, and when that was the
pair's first sample the way the turn went, the next detent moved it one frame to the pair's other sample
(seen on ZSIN: 124 then 125, 195 then 196, 266 then 267). PP08 skips the pair under the point. The same
run, one detent at a time and settled: PP07 54, 124, 124, 125, 195, 196, 196, 266, 267; PP08 54, 124,
124, 195, 266, 337, 337, 408, 479 (a repeat is a detent stock's encoder hysteresis swallowed).

Locks (LK03, the same main OS as PP08), ZSIN on BD:

| Check | Result |
| --- | --- |
| GRID RECORDING, trig on step 1, held with FUNC, STA turned 4 to 14 detents, then played | the first detents make the lock at the sound's value, as stock; then the lock steps 54, 195, 266, 408, 479 (frames), each a crossing, in order (detents between fell to hysteresis) |
| LIVE RECORDING, FUNC + STA turned while it records | the sound's STA steps 54, 124, 195, 266 and the locks recorded on the steps play those frames, all crossings |
| plain turn with a trig held | stock's fine steps |

Stock edits a held trig's lock in one of two ways: through the view's lock editor when there is one
(0x4003a19c's call; not reached in these runs), or, without one, trig by trig through a callback that
reads each trig's lock (view vt + 112) and adds the turn (view vt + 120). Pipo Loop steps each trig's
lock from its own value at that callback (0x40037f6c), so several held trigs each go to their own next
crossing. The editor path is hooked the same way but was not exercised. LIVE RECORDING records the
sound edit, which the 0x4003a240 site already snaps.

## Sample-exact turns and fine offsets: findings (2026-10-27)

- **Sample-exact PIPO turns.** Stock's prefetch builds each block's loop wrap a block ahead: two
  8-frame-aligned eDMA segments (the loop's tail, then its start) into a 140-frame SRAM window that the
  resampler reads straight through. A turn exactly at END would need the second segment replaced by a
  reversed copy of the tail after the DMA lands and before the render reads it, in both of stock's
  prefetch routines (voice 0 inline, voices 1-7 in a loop), and the position, phase and direction put
  right the next block. That is several new sites inside the audio interrupt and DMA timing the emulator
  may not show as the unit does. Not built.
- **Exact zero snap past 30,720 frames.** Prototyped (FO01-FO03) and dropped at npp1993's choice (his
  precise loops are short); the 0.64 s limit stays. Found on the way: reading the kit's sounds from an
  `ev_project` handler broke the project load at boot in rytm-emu; reading them from `ev_tick` did not.
  The design: a per-sound fine offset: 16 bits a sound of
  core-ar2's sound storage (an 8-bit offset and an 8-bit tag of the STA or END value it belongs to, for
  each), saved with the sound and kit; added to the start and end frames where stock computes them (the
  returns of 0x4011bf44 at 0x4011c2a8 and 0x4011c850, both free), and only while the effective value
  still equals the tagged one, so locks, scenes and the LFO play stock's grid. Stock OS ignores the
  storage and plays the 1/30,720 grid. Not built.

## Exact turns and exact snap (2026-10-28)

**Exact turns (PP09).** Stock's prefetch copies each block's 140-frame window by DMA, a block ahead;
near the loop's end it copies the frames up to the end, then the loop's start, so the voice wraps at
the exact frame. For a voice in PIPO the module points that DMA's source at a buffer of the loop's
frames in reverse order (at the top: the segment after END; at the bottom, read backward: the main
segment), so the voice plays straight on into the way back; the block after, it moves the position
stock wrapped to the mirrored frame, 8 frames on (the play position runs 8 frames ahead of the frames
heard, at every pitch tried), and turns the voice round. rytm-emu, ramp on BD: period 48,000 frames
for a 24,000-frame loop, turns symmetric to within a few units; TUN +24 period 6,000, about -24
95,965 (96,000 ideal); with END inside the sample every turn's largest step is 1; with END on the
sample's last frame the first turn after a hit can be up to 7 frames off (no room past the end).
SD (voices 1-7 routine) ping-pongs too. Not provable in the emulator: the data cache (the module
pushes its buffer's lines; rytm-emu models no cache) and the unit's DMA timing.

**Snap on stock's own frame.** Stock maps STA and END to frames with an EMAC multiply by 8/15 that
works out to ceil(frames * value / 30720) for every value up to 30,720 frames (checked by working the
instructions through for 400,000 cases); the module had assumed the floor, so snapped points played
one frame after the crossing. PP09 uses stock's mapping and lands on the first sample of the new sign:
on ZSIN, sound and locks alike play from crossings exactly (sound: #3-#6; locks: #1, #4, #5). One
crossing a detent: core-ar2's ev_enc counts the encoder (8 counts a detent); four 2-count events step
one crossing, as one 8-count event does. With LOP ON, END stepped from 23,647 down by whole periods
(rising crossings only), matching STA's rising crossing.

## Zero snap holds; shared crossings and edges (2026-10-28)

With FUNC held a turn on STA or END only ever lands on a crossing: where none is left the way it
goes, the point holds (PD01 had passed such a turn to stock, which stepped off the crossings). STA
and END may pass each other and may share a crossing (npp1993's rule). rytm-emu, PP10, ZSIN on BD
(24,000 frames, not at zero at its first or last frame), settled values as frames, `d`/`u` the
crossing's direction:

- STA, LOP ON, END at 120: right 54d 125u 196d 267u 337d, left back to 54d, then held at 54 for the
  remaining detents (frame 0 is not at zero, so it is no crossing).
- END: left 23931u 23860d 23789u 23718d 23647u, right back to 23931u, then held (the sample's end is
  not at zero).
- END at 0, STA on 196d, LOP ON: END right 54d 196d 337d 479d …: only falling crossings (STA's
  direction), onto STA's own crossing, then past it.
- A trig's lock (GRID RECORDING, sound STA = END = 196d, LOP ON): the lock's STA left to 54d and held
  there; right from 54d to 196d (END's crossing), 337d, 479d, 621d.
- A loop of no length (STA = END = 196) with LOP ON and with PIPO, BD hit twice: plays without a fault
  for the whole run; the voice stays parked around frame 196, LOP ON and PIPO alike (PIPO leaves
  loops under 160 frames to stock).

Stock glides STA and END to a new value rather than jumping: PP10, effective STA read every 1M
instructions during a FUNC turn from 0 to ZSIN's first crossing went 0x00, 0x02, 0x0f, 0x1a, 0x22, …
0x41, 0x42, 0x43 and settled on 0x44 about 30M instructions later (the same on the next step, 0x47
… 0x9f). A wrap or PIPO turn inside that glide plays from a point between crossings, which matches
npp1993's report of clicks only while turning (PD01/PD02 era, PIPO_110). Nathaniel chose to keep
stock's glide (2026-10-28).

The emulator drops the first two to four encoder events after FUNC goes down or the turn changes
direction (stock's encoder smoothing in rytm-emu), so the step counts above are not one a turn.

PD02's counters (rytm-emu, ZSIN on BD in PIPO, STA 0 and END 120, after one hit): PIPO turns 4,
mirrored segments 16, unmirrored turns 0 500M instructions after the hit; 17, 68 and 0 after 1,800M.
Counting idle voices too had shown thousands of unmirrored "turns": before its first trig a voice
reads stock's silence buffer and its position wanders, so PD02 counts playing voices only.

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

### PD01 on an Analog Rytm MKII (2026-10-28), as reported

npp1993 flashed PD01 (`6ba6b5cc…9134`; PC07's code with one crossing an encoder event, plus a readout
of encoder counts, events, crossings moved and the last snapped frame while FUNC is held), loaded the
test samples PIPO_SLOW, PIPO_110 and PIPO_SQR and reported:

- A, detents: a slow detent gave one tick. FUNC + STA snapped onto PIPO_SLOW's zero crossings well.
  FUNC + END near the sample's end sat near two crossings but not exactly on them. Readouts (counts,
  events, crossings, frame), END turned left one slow detent at a time: -104 711 1 22340; -107 714 1
  22340; -108 715 1 22340; -110 717 1 22340; one fast flick: -170 721 1 22340; another: -303 731 -4
  10820. Frame 22340 is one before PIPO_SLOW's crossing at 22341: a backward snap took the last
  quiet sample, not the first signed one, and the next left turn found the same crossing again.
  Fixed after PD01 (the backward snap lands on the first signed sample, as the forward one does).
- B, line against frame: the STA and END lines on SMPL WAVEFORM sat about one column off at times.
  At the left edge, FUNC + turn left moved STA off the crossing; STA and END moved toward each other
  on neighbouring crossings went off the crossings. Both were the module passing a turn with no
  crossing to stock's whole-number step; fixed (see above). The line is stock's drawing: the module
  never draws it, and one column covers about 225 frames of a 0.6 s sample, so a line on a crossing
  can sit a column off; the frame readout is what plays.
- Later on PD01 (PIPO_110): plain points with LOP ON click; FUNC-snapped points with LOP ON are clean;
  holding FUNC and turning STA while the loop plays gives occasional clicks, clean once it lands;
  PIPO, once settled, gives a muffled knock at the turns, not a discontinuity.
- C, clicks (PIPO_110): LOP ON with STA and END not snapped clicked on every pass; snapped, LOP ON
  was clean; PIPO gave a soft knock at the turns. At a turn the waveform's slope reverses, which no
  choice of frame avoids on a pure tone, so this is expected (README).

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
| 0x4011c58a / 0x4011c5e6 | 6 / 8 | voice 0's prefetch: where the DMA sources of the segment after the loop's end and of the main segment are written; exact turns swap in the mirrored frames | stock guards in build.json |
| 0x4011cb2e / 0x4011cb98 | 10 / 8 | the same in the voices 1-7 prefetch | stock guards in build.json |
| 0x4003a19c | 6 | the setup of the edit handler's lock call (a held trig, with a lock editor): for STA and END with FUNC held, the step from the lock to the next crossing | `8b2ff131…1acf` |
| 0x40037f6c | 6 | in stock's per-trig lock edit, the setup of the call that adds the turn to a trig's lock: for STA and END with FUNC held, the step from that trig's lock to its next crossing | `6bec112e…370e` |
| 0x4003a240 | 6 | in stock's encoder handler, the setup of the view's apply-delta call; for STA and END with FUNC held it replaces the whole-number step by the step to the next zero crossing, sent as a fine turn | `87377367…3e20` |
| 0x400fedf4 | 6 | the first instruction of stock's shared OFF/ON switch icon; for LOP's record (ctx 0x419a5770, seen on a breakpoint on stock) it tail-calls BD CLASSIC WAV's three-position icon routine 0x400fef4a with the same arguments, else stock | `0528305f…fac3` |

Modwerk's linker links Pipo Loop beside Filter Hz, Loop Sync and Silent Pad+Trig together (PX07).
None of the eleven sites is patched by Silent Pad+Trig (0x400c772e, 0x400c7a3e, 0x400c7e90,
0x400c7950, 0x400ec184), Loop Sync (0x40121020) or Filter Hz. The OFF/ON text routine is shared by
parameter ids 34-37, 45 (LOP), 224, 248 and 463; the switch icon by twelve parameters (those and 73, 414, 415, 434).

## Load and memory

- Memory: 7,221 bytes in the shared mod area (4,393 of code and text, 2,828 of state, 2,560 of it the mirror buffers), from PP09's package. The icon and zero-snap gates run in the UI task only, not in the audio path.
- Load: static estimate from the compiled handler: about 15 instructions a block for a voice with LOP
  OFF or no track, about 70 for one looping with ON, about 105 for one in PIPO, plus four 32-bit
  divisions when a voice's STA, END or sample change. With the Rytm guide's emulator figure (0.023 %
  of a block per instruction a frame), about 0.1 % with nothing looping and about 0.6 % with all eight
  voices in PIPO. rytm-emu's breakpoints and profiler do not reach the audio interrupt's mod code, so
  it was not measured. Not measured on a unit.

## Open items

- Possible later improvement (npp1993, 2026-10-28): FUNC snaps could skip stock's STA/END glide and
  jump straight to the crossing, so no wrap or PIPO turn plays from between crossings while a turn
  settles. Not built: STA and END as played are stock's smoothing plus LFO and velocity modulation,
  so the module would have to work out the target itself every audio block, only after a FUNC snap,
  without breaking plain-turn smoothing or modulation: more code in the audio path and another
  round of emulator and unit tests.

## Not tested

- On a unit, beyond the PC03 and PD01 reports above: PC08, PP10 and PD02 have not been flashed.
- KIT SAVE and KIT RELOAD from the KIT menu, the sound browser, sound SysEx dumps and Overbridge.
- Eight voices in PIPO at once, LFO or velocity modulation of STA and END while in PIPO, and Loop
  Sync and Pipo Loop on the same track.
- PX07 booted.
- Several trigs held at once while snapping locks; the lock-editor path (0x4003a19c).
