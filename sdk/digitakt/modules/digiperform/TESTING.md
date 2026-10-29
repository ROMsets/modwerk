# Digi Perform testing

## In Modwerk

Evidence tier: `none`. Nothing here is a filed hardware report; the author's own observations on a unit are listed below as they were reported.

Source: `sdk/digitakt/modules/digiperform/src` at commit `da4d96e41d3b34a7edf2940e189db19c14fa7725`. Modwerk's pinned toolchain compiled the source twice and produced the source-only packages in `sdk/imports/packages/digiperform/` (no stock bytes: addresses, lengths and SHA-256 guards only). The vendored elekloader builder (kit 0.6.0) then linked core 2.1 and Digi Perform into a local image on each release, with the owner's stock OS files, which stay on the author's computer:

- OS 1.54: core 2.1 and Digi Perform 0.1.0-experimental, built and verified; image SHA-256 `9903ee834ae91e79198e26b3a74850acc0256328b4985cd5ec2a5c3c67ce3d01`. Digi Perform package SHA-256 `0609ca6128005e1ea33dcdfcd445cbc8de6123a0cf928c45d1f0fdeb9c8dd16c`.
- OS 1.53: core 2.1 and Digi Perform 0.1.0-experimental, built and verified; image SHA-256 `fd713938a9b0e75df20782782071978eca39197b307ecf49460e9ebe8762e46d`. Digi Perform package SHA-256 `2ad8bdc1dd676e2b38ce474dcee1b85a16cdcb34364b9b5d77f3a840c9c6dd8a`.

The local images carry the build label `3.0d`. Both are build checks; the author's hardware observations are listed below.

## Resources

- Memory: 18,060 B per release: 17,242 B in `.run`, 798 B in `.bss` and 20 B of table contributions (five core events). No FAST AUDIO bytes. This is the object's size, from the package; it excludes the shared core and linker alignment.
- Load: not measured. The engine hook runs once per audio block and adds up to eight offsets to the smoothed parameter words; the cost has not been timed on a unit or in a cycle-accurate run.

## Combinations

`modules:check` flags the overlaps the manifests declare: Digi Perform and Digi Matrix patch the same render site, so it is declared in `compatibility.conflicts`. Digi Perform and cc-names no longer share a patch site. The elekloader combination matrix with every other Modwerk mod was not run.
Digi Perform beside Digi Poly and DigiChain (build `3.0d` with Digi Poly 2.0, DigiChain 1.6 and core 2.1; emulator only, never hardware evidence). Digi Poly replaces the draw slot of the TRIG page's view, next to the key slot Digi Perform patches. Both releases link into one image with no overlap reported. In the pinned digiemu (OS 1.54 and OS 1.53), with the audio engine off:
- On a track with the stock machine, the TRIG page drew as stock, TRIG pressed again opened the Perform page, and TRIG left it back to the stock TRIG page.
- On a track set to POLY (FUNC + SRC, then POLY), the TRIG page drew Digi Poly's page (NOT1 to NOT4 as a keyboard, LEV, VEL, LEN, PROB, LFO.T). TRIG pressed again opened the Perform page, TRIG left it back to Digi Poly's TRIG page, and opening Perform a second time worked the same way. NO on the Perform page stayed on it, as documented.
- Not tested: audio with POLY notes while the Perform page is open, assignments or pattern changes with Digi Poly active, Digi Poly's other pages, and any run on a unit.

## Emulator checks (author)

Run in the pinned digiemu (revision `c1b5735835923e328f8b4950d6ba927875e5b669`, patched Unicorn) with the audio engine on, in a network-denied container, with the same panel inputs as a unit would receive. These are emulator results, never hardware evidence.

- Both releases: the page opens from the TRIG page, draws its eight knobs, accepts Perform Quick Assign and Perform Edit, and stores the assignments in the pattern's kit. OS 1.54 was exercised throughout; on OS 1.53 only the page, the pattern-change release and the metronome and scale screens were run.
- The engine's smoothed parameter words move by the knob's offset and return when the page is left, the pattern is reloaded or NO is pressed.
- FUNC and turn lands on the parameter's snap values, and the lock after each snap matches the same clicks on a stock page; the track and parameter choosers step like the SAMP selector.
- Pattern change: stopped, the offsets reload at once; playing, they hold at the selection and release when the engine switches pattern.
- Stock screens: FUNC + TRK, FUNC + PTN, FUNC + PAGE, and a held step with an arrow show the stock screens, the same as stock without the module. The stock firmware itself keeps the metronome and scale pages open after the keys are released and closes the metronome on a second FUNC + PTN; the Perform page leaves first, so those behaviours are the stock ones.

Storage and entry changes (build `3.0a`, emulator and host only; none of it is hardware evidence; builds `3.0c` and `3.0d` differ only in the decode described under "Overbridge delay"):

- The assignments now live in tracks 5 to 8's spare sound bits with a version of their own and a marker in parameter slot 49 of those tracks. A host test of the packing code taken from `src/perf.c` ran 20,000 random pools: every entry (all-tracks flag, track, parameter, direction) read back as written, the two reserved bits were left as they were, only the pool's 92 own bits changed (no other byte or bit of the sound, tracks 1 to 4 untouched), the 92 stream bits are 92 distinct bits, and a pool without version 1 reads as empty.
- All eight rows of Perform Edit were set to different tracks and parameters, and the Perform page and the edit list read every one back, including rows G and H, which sit across the track 7 and 8 boundary of the pool; earlier rows did not change when later ones were set. A negative direction read back. The all-tracks setting was not reached in the emulator run.
- Pattern A01 and A02 kept their own assignments across a pattern switch, and copying A01 to A03 carried both.
- With Euclidean 1.1.5 and cc-names 2.1.1 in the same image (dt-support 1.0.1), the elekloader builder reports no overlaps on both releases. Euclidean's page (hold FUNC + AMP), the Perform page (TRIG twice) and Euclidean again all opened in turn on OS 1.54 in the emulator. On a MIDI track the second TRIG press opened the Perform page, as it did before the MIDI hooks were removed.
- Not run: a project save and reload (the capture tool's card is write-protected, and a plan with my own harness landed on the wrong screens), loading a sound onto tracks 5 to 8 or onto tracks 1 to 4, saving a track 5 to 8 sound to the pool, the same checks on OS 1.53, and any hardware run of the new layout.

## Overbridge delay (builds 3.0c and 3.0d)

Build 3.0a decoded all eight assignments bit by bit on every audio block. The author reported (OS 1.54, Digitakt connected to Overbridge, after switching patterns) a noticeable delay between turning an assigned knob and the parameter changing, not seen on the earlier submitted version and not seen with Overbridge disconnected. Build 3.0c decoded the pool only when its 92 bits had changed; the author reported "very much improved" (OS 1.54 with Overbridge; reported, not timed). The author then reported that the delay when switching patterns, and the value indicators showing the current values, had come back, less than before but still noticeable. Build 3.0d keeps a separate decoded copy for the UI and for the audio hook (3.0c shared one, which could be overwritten by the other side while a pattern change was pending) and syncs once per audio block. A host test compared the new decode with the old bit-by-bit decode on 200,000 random states, cache reused, and found no difference; it says nothing about speed or timing. The cost per block was not measured on a unit or in a cycle-accurate run, and no build was run side by side with another on one unit. 3.0d was not exercised in the emulator beyond the page drawing and a pattern switch while playing.

Known limitation, reported by the author: when a new pattern starts to play, the value indicators briefly show the previous pattern's value and then update to the new pattern's. The cause is not confirmed; the engine's smoothed parameter words probably still run from the old sound to the new one when the knobs pick them up. A build that read the new sound's stored value instead (3.0e) made, in the author's words, "not much difference", and was dropped. The flash could not be reproduced in the emulator.

## Author's hardware observations

Reported by the author (ROMsets) while testing local builds on a unit, build labels in brackets. These builds (2.x) kept the assignments in other bytes of the kit than the current layout (tracks 5 to 8's spare bits, build 3.0a), so none of this, except the first item (3.0a), covers the storage as it now is. Recorded as reported. The author says most of this testing was on OS 1.54; the per-item OS release was not recorded, and none of it is a version-matched hardware report.

- Build 3.0a on OS 1.54, the checklist given for this update (assign, save and power-cycle, pattern switch and copy, sound loads onto tracks 5 to 8 and 1 to 4, a MIDI track's TRIG page, the Euclidean and cc-names image): the author reported "everything seems good" apart from the Overbridge delay under "Overbridge delay" above. Reported as a whole; the individual steps were not itemised. Build 3.0d on OS 1.53, a smoke test of the same functions on a unit: the author reported "I think we're good to go". Reported as a whole; the individual steps and the pass or fail of each were not itemised. Build 3.0d on OS 1.54 was tested by the author with Overbridge connected (see "Overbridge delay").
- FUNC and turn snapped much more sensitively than a stock page (before the lock fix), then "a load better" after it (2.2a).
- Track and parameter chooser resolution "right for both" after matching the SAMP selector (2.2b).
- Selecting a pattern while it plays: the parameters did not reset to the pattern's values at first (2.2d), then "that works now" (2.2e).
- Chord setup, metronome, scale, microtiming and retrig screens "look good" (2.3c).
- Overall, "hardware looks pretty good"; a slight delay in the knob indicators after a pattern change was reported, and a change to show the new values at the switch followed (2.3a, not re-reported).
- OS 1.53: a final smoke test on a unit (build 2.3c) that the author describes as covering a good part of the main functionality: the page opening, Perform Quick Assign, setting and editing assignments in Perform Edit, and changing assigned parameter values with the knobs. Not a full pass.

Not tested on a unit, or not reported step by step: the current storage layout's checklist above was reported only as a whole, so each step (power-cycle persistence, sound loads onto tracks 5 to 8 and 1 to 4, the all-tracks setting) has no item of its own; a project saved with assignments loaded on a build without the module; CPU load (never timed), every combination with other mods, and the stock flows compared side by side beyond the screens above.

## Category checklist (system)

- Put the setting where it belongs: PERFORM EDIT is a row in SETTINGS; the page is the second page of TRIG. Done.
- A way to turn it off that restores stock: leaving the page puts every parameter back; not installing the module restores stock. Compared on the emulator for the screens above only.
- Defaults are stock behaviour: no assignment exists in a new project, so nothing moves until one is made. Emulator only.
- Persistence: stored in the sound records' spare bytes. Pattern and kit copy carry it in the emulator; power-cycle survival is not tested.
- A project with assignments loaded without the module: not tested.
- Affected: the original Digitakt on OS 1.53 and 1.54, audio tracks 1–8; existing projects are unchanged until an assignment is made.

## Documentation capture — 28 October 2026

Documentation version: `0.1.0-experimental`. Source revision: `ef0a333cf01282ecd0ae896abb878cf71b38bf9a` (`sdk/digitakt/modules/digiperform`). The OS 1.54 image above was captured through real panel inputs in the pinned digiemu with firmware-native drawing (`hle=False`) and a disposable first-boot project. The capture script runs without the audio engine, so the knobs show their assignment but not live values. Only complete, unmodified LCD frames were kept at 6× integer scale, and every image was opened for review. The record is [capture.json](media/capture.json); the plan is [capture-plan.txt](media/capture-plan.txt).

- The Perform page with eight empty knobs.
- Perform Quick Assign: choosing the track for knob A.
- Knob A assigned to track 1's TUNE.
- SETTINGS > PERFORM EDIT.
- Perform Edit with row A set.

UI-only documentation; no hardware, audio, timing, persistence or stress qualification. Reproduce with your own OS file, keeping firmware and emulator outputs outside the checkout:

```sh
python3 scripts/capture-digi-module-ui.py \
  --emulator /path/to/pinned/digiemu \
  --firmware /private/capture/custom.syx \
  --plan sdk/digitakt/modules/digiperform/media/capture-plan.txt \
  --out /private/capture/new-session
```

Run that inside the isolated sandbox described in [MODULE_UI_CAPTURES.md](../../../../docs/MODULE_UI_CAPTURES.md).
