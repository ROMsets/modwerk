# Digi Perform testing

## In Modwerk

Evidence tier: `none`. Nothing here is a filed hardware report; the author's own observations on a unit are listed below as they were reported.

Source: `sdk/digitakt/modules/digiperform/src` at commit `21e098a73f442daff0fc82e7c437b6731179f4dd`. Modwerk's pinned toolchain compiled the source twice and produced the source-only packages in `sdk/imports/packages/digiperform/` (no stock bytes: addresses, lengths and SHA-256 guards only). The vendored elekloader builder (kit 0.6.0) then linked core 2.1 and Digi Perform into a local image on each release, with the owner's stock OS files, which stay on the author's computer:

- OS 1.54: core 2.1 and Digi Perform 0.1.0-experimental, built and verified; image SHA-256 `917b882f3acdde69d6dbc7b9a06f192dd3a23bc2c4d655a744dbc060b2f7b2bd`. Digi Perform package SHA-256 `9337a7f099aa7f5c87ee3dadfb63e6571c8e8803e5fb4296270b29b83c3628a0`.
- OS 1.53: core 2.1 and Digi Perform 0.1.0-experimental, built and verified; image SHA-256 `017c016a29512b07b48b0d60c5dde36d64d7e3fe294e4e0783a17020771463bb`.

The local images carry the build label `2.3c`. Both are build checks, not hardware tests.

## Resources

- Memory: 17,292 B per release: 16,546 B in `.run`, 726 B in `.bss` and 20 B of table contributions (five core events). No FAST AUDIO bytes. This is the object's size, from the package; it excludes the shared core and linker alignment.
- Load: not measured. The engine hook runs once per audio block and adds up to eight offsets to the smoothed parameter words; the cost has not been timed on a unit or in a cycle-accurate run.

## Combinations

`modules:check` flags the overlaps the manifests declare: Digi Perform and Digi Matrix use the same hook site and kit bytes, and Digi Perform and cc-names replace the same pointer in the MIDI track page's view, so both are declared in `compatibility.conflicts`. The elekloader combination matrix with every other Modwerk mod was not run. Not tested.

## Emulator checks (author)

Run in the pinned digiemu (revision `c1b5735835923e328f8b4950d6ba927875e5b669`, patched Unicorn) with the audio engine on, in a network-denied container, with the same panel inputs as a unit would receive. These are emulator results, never hardware evidence.

- Both releases: the page opens from the TRIG page, draws its eight knobs, accepts Perform Quick Assign and Perform Edit, and stores the assignments in the pattern's kit. OS 1.54 was exercised throughout; on OS 1.53 only the page, the pattern-change release and the metronome and scale screens were run.
- The engine's smoothed parameter words move by the knob's offset and return when the page is left, the pattern is reloaded or NO is pressed.
- FUNC and turn lands on the parameter's snap values, and the lock after each snap matches the same clicks on a stock page; the track and parameter choosers step like the SAMP selector.
- Pattern change: stopped, the offsets reload at once; playing, they hold at the selection and release when the engine switches pattern.
- Stock screens: FUNC + TRK, FUNC + PTN, FUNC + PAGE, and a held step with an arrow show the stock screens, the same as stock without the module. The stock firmware itself keeps the metronome and scale pages open after the keys are released and closes the metronome on a second FUNC + PTN; the Perform page leaves first, so those behaviours are the stock ones.

## Author's hardware observations

Reported by the author (ROMsets) while testing local builds on a unit, build labels in brackets. Recorded as reported. The author says most of this testing was on OS 1.54; the per-item OS release was not recorded, and none of it is a version-matched hardware report.

- FUNC and turn snapped much more sensitively than a stock page (before the lock fix), then "a load better" after it (2.2a).
- Track and parameter chooser resolution "right for both" after matching the SAMP selector (2.2b).
- Selecting a pattern while it plays: the parameters did not reset to the pattern's values at first (2.2d), then "that works now" (2.2e).
- Chord setup, metronome, scale, microtiming and retrig screens "look good" (2.3c).
- Overall, "hardware looks pretty good"; a slight delay in the knob indicators after a pattern change was reported, and a change to show the new values at the switch followed (2.3a, not re-reported).
- OS 1.53: a final smoke test on a unit (build 2.3c) that the author describes as covering a good part of the main functionality: the page opening, Perform Quick Assign, setting and editing assignments in Perform Edit, and changing assigned parameter values with the knobs. Not a full pass.

Not tested on a unit, or not reported: power-cycle persistence of the assignments, a project saved with assignments loaded on a build without the module, anything on OS 1.53 beyond the smoke test above, CPU load, every combination with other mods, and the stock flows compared side by side beyond the screens above.

## Category checklist (system)

- Put the setting where it belongs: PERFORM EDIT is a row in SETTINGS; the page is the second page of TRIG. Done.
- A way to turn it off that restores stock: leaving the page puts every parameter back; not installing the module restores stock. Compared on the emulator for the screens above only.
- Defaults are stock behaviour: no assignment exists in a new project, so nothing moves until one is made. Emulator only.
- Persistence: stored in the sound records' spare bytes. Pattern and kit copy carry it in the emulator; power-cycle survival is not tested.
- A project with assignments loaded without the module: not tested.
- Affected: the original Digitakt on OS 1.53 and 1.54, audio tracks 1–8; existing projects are unchanged until an assignment is made.

## Documentation capture — 26 October 2026

Documentation version: `0.1.0-experimental`. Source revision: `21e098a73f442daff0fc82e7c437b6731179f4dd` (`sdk/digitakt/modules/digiperform`). The OS 1.54 image above was captured through real panel inputs in the pinned digiemu with firmware-native drawing (`hle=False`) and a disposable first-boot project. The capture script runs without the audio engine, so the knobs show their assignment but not live values. Only complete, unmodified LCD frames were kept at 6× integer scale, and every image was opened for review. The record is [capture.json](media/capture.json); the plan is [capture-plan.txt](media/capture-plan.txt).

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
