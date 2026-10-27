# CC Recall testing

Module version 0.1.0-experimental, Digitakt I OS 1.54 (stock file SHA-256 `f78ba80f…3cf6`), Core 2.1. Firmware, linked images and emulator state stay on the developer's computer; none of it is committed.

## Evidence tier

Emulator only so far. Hardware steps for the author are in section 5; no hardware result is recorded yet.

## How it works (OS 1.54)

**Stock.** The MIDI tracks' task keeps its own copy of each MIDI track's parameters and a pointer to the pattern they come from. Reloading or changing the pattern, loading a project or pressing STOP gives the task the pattern again: either through 0x400e0b04, which also marks all eight tracks to be reloaded at their next trig, or through 0x400e0b3a, which reloads them at once. At a track's first trig after the mark, a note or a lock trig, the task copies the track's parameters from the pattern and sends all of them, forced: the CC VAL slots with a CC SEL, mod wheel, breath, pitch bend and aftertouch. Until that trig nothing is sent, so external gear and the effects through FX CONTROL CH keep the old values.

**The module.** Two 6-byte `jsr` sites replace the instruction in each function that stores the pattern pointer (0x400e0b08 and 0x400e0b56); both replay the store and count a new pattern. A third replaces the test at 0x400e128c in the task's clock record, which arrives about 30 times a second when stopped and more often while playing. With CC RECALL ticked, from the clock after a new pattern on, the handler loads one MIDI track a clock from the pattern and makes the same calls for parameters 18-32 as the stock first trig, forced, then puts back the track's mark so its next trig still loads and sends as stock. Tracks with CHAN off send nothing and are skipped within the same clock. The handler ends by replaying the replaced test, whose flags the next branch reads. One track a clock keeps each clock's messages (at most 12 per track) well inside the task's output buffer.

## 1. Stock behaviour (pinned digiemu c1b5735)

MIDI track A, CHAN on (channel 1), a trig on step 1, the project saved to slot 2. VAL1 raised by about 36 while playing, then the action. MIDI OUT captured with digiemu's MidiOut; the task's loads logged at 0x400e0918.

| Action | MIDI OUT |
| --- | --- |
| Reload (FUNC + NO) while playing | nothing until the next trig of track A, then the note with `e0 00 40`, `d0 00`, `b0 01 00`, `b0 02 00`, `b0 00 00` (VAL1, back to the saved 0) and the other slots |
| STOP, reload, PLAY | nothing until PLAY; at the first trig the same messages |
| The same with a lock trig on step 1 instead of a note | the same messages at the lock trig, without the note |
| Reload while stopped, no PLAY | nothing |

So stock already sends a reloaded pattern's values, but only with each track's next trig.

## 2. Running firmware (pinned digiemu c1b5735, firmware-native drawing)

Same setup, with this module, CC RECALL ticked before the project was saved.

| Build and phase | MIDI OUT |
| --- | --- |
| Module, ticked: STOP | at once: `e0 00 40 d0 00 b0 01 00 b0 02 00 b0 00 23 …` (the values in use, VAL1 0x23) |
| Module, ticked: reload while stopped | at once: the same messages with VAL1 `b0 00 00`, the saved value |
| Module, ticked: PLAY afterwards | the first trig sends the note and the same values again, as stock |
| Module, ticked: project load (LOAD PROJECT, slot 2, while playing) | the project's values at once, after the stock reload of all tracks |
| Module, unticked: STOP, reload, PLAY | stock: nothing until the first trig |
| Core 2.1 + this module + FX Track 0.1.0, both ticked, FX CONTROL CH 1, SEL1 24 (reverb PRE), saved VAL1 36, raised to 80 with FX TRACK unticked, reload while stopped | reverb PRE 8 → 36 within one 25 ms read; CC 24 does not leave MIDI OUT, the other slots do |
| The same with only FX TRACK ticked | reverb PRE stays 8; nothing is sent (stock) |

Build `.syx` SHA-256: module alone `0f79fa5e…69a70a`; with FX Track `20419504…a68b54`. No halt.

## 3. Build and rules

- Memory: 560 bytes of code and 14 bytes of BSS in DDR; no fast SRAM.
- Load (emulator): the clock-record handler timed with digiemu's CycleClock at 250 MHz, zero-wait memory. Ticked and idle: 17-24 cycles per clock over 297 clocks. A clock that loads and sends one track: about 5,220 cycles (about 21 µs). A clock that only skips tracks with CHAN off: about 1,630. No code in the audio interrupt. Not a hardware measurement.

## 4. Screens

[capture.json](media/capture.json): the SETTINGS row unticked and ticked, from the module-alone build in the pinned emulator.

## 5. Hardware steps (for the author)

Not yet tested. Builds: module alone `0f79fa5e…69a70a`, and with FX Track `20419504…a68b54`.

1. CC RECALL appears in SETTINGS, unticked.
2. Setup: a synth (or a MIDI monitor) on MIDI OUT; MIDI track A CHAN on its channel, SEL1 its cutoff CC, a trig on step 1. Save the project.
3. Unticked: turn VAL1, stop, reload the pattern (FUNC + NO). The synth keeps the turned value until PLAY reaches the trig (stock).
4. Ticked: the same. The synth jumps back to the saved value as soon as you reload, without PLAY.
5. Ticked, playing: change to another pattern with different CC VAL values on track A. The synth follows when the new pattern starts, before track A's first trig.
6. Ticked: load the project from SETTINGS > PROJECT. The synth follows at once.
7. Ticked: play a pattern with a lock on VAL1 and press STOP just after the locked step. The synth goes back to the pattern's value at once.
8. With FX Track: the FX Track tutorial setup (MIDI track H on FX CONTROL CH 16, SEL1 85, delay TIME). Turn VAL1, reload: the delay TIME returns to the saved value at once.
9. Several MIDI tracks with CHAN on: all of them follow after a reload.

## Stock flows compared

| Flow | Result |
| --- | --- |
| MIDI output with CC RECALL unticked | Stock (section 2) |
| The first trig after a reload, pattern change or STOP | Still loads and sends as stock |
| Ticking the row | Sends nothing |
| SETTINGS menu | Stock rows and order unchanged; one row added at the end |
| Power cycle | Row unticked again |
| Pattern change while playing, song mode | Not tested |

## Still to test

- Everything in section 5 on hardware.
- Pattern change while playing and song mode, in the emulator too.
- USB as the only output port.
