# Pipo Loop

An Analog Rytm MKII mod by npp1993 (Nathaniel Pavel). Version 0.1.0-experimental, OS 1.74 with core-ar2 0.3.

## Overview

Pipo Loop gives the SAMPLE page's LOP parameter a third value, PIPO, after OFF and ON. A sample whose LOP is PIPO plays its loop forward from STA to END, then backward to STA, then forward again, for as long as the note lasts: a ping-pong loop. ON stays the stock forward loop, which jumps back to STA at END, and OFF plays the sample once.

PIPO is an ordinary LOP value. It saves with the sound and the kit, takes parameter locks and scene locks, and comes back after a power cycle. A sound saved with PIPO plays as a normal forward loop on stock OS 1.74 or a build without this mod, because stock reads any nonzero LOP as ON.

The turns are smooth (no jump in the waveform). Each turn falls on the audio engine's 32-frame block, so it comes up to about one block, plus 8 frames, before STA or END.

![LOP reads PIPO on the SAMPLE page](media/lop-pipo.png)

## Controls

| Control | Where | Values |
| --- | --- | --- |
| LOP | SAMPLE page, encoder G | OFF, ON, PIPO. OFF and ON are stock. PIPO plays the loop between STA and END forward, then backward. |

The knob's switch icon draws PIPO like ON; the value text shows PIPO while you turn it. PIPO applies only to the sample layer of a sound, as LOP does.

## Usage

Use a build containing Pipo Loop, select a track whose sound plays a sample, and press SAMPLE. LOP is encoder G.

### Quick tutorial

1. On the SAMPLE page, turn LOP past ON to PIPO and play the sound: the loop between STA and END goes back and forth.
2. Set STA and END around a sustained part of the sample (a pad, a vowel, a held note) for a seamless back-and-forth drone. Lengthen the AMP envelope to hear several turns.
3. Hold a trig in GRID RECORDING and turn LOP to PIPO to ping-pong only on that step; the other steps keep the sound's ON or OFF. In SCENE mode, hold a pad and turn LOP to put PIPO in that scene.

![LOP locked to PIPO on a step](media/plock-pipo.png)

![LOP locked to PIPO in scene 1](media/scene-pipo.png)

### Changes to stock flows

- **LOP has a third value.** What changes: turning LOP clockwise past ON now reaches PIPO; locks and scenes can hold PIPO. Why: a musician looks for loop behaviour on LOP, and a third value needs no new page, key gesture or menu row (a FUNC gesture on the SAMPLE page was considered, but FUNC + STA, END and LOP are stock's coarse steps and FUNC + TUN and FIN belong to Loop Sync). What you see: the value text reads PIPO; the switch icon draws it like ON. It applies only in a build with this mod; leave LOP on OFF or ON for stock behaviour. Checked beside it: OFF and ON play exactly as stock, LOP's p-locks and scene locks, kit save through power-off, MIDI CC 30.
- **MIDI CC 30 (LOP).** Stock writes CC 30's value into LOP unchanged (0 is OFF, anything else ON). With this mod a CC value of exactly 2 gives PIPO; every other value does what it did.

## Compatibility and limitations

- OS 1.74 only, with core-ar2 0.3. No other module is needed, and it builds beside Silent Pad+Trig, Filter Hz and Loop Sync (no shared sites).
- Sites: the audio interrupt's call to the voice 0 sample prefetch at 0x40121134 (a `jsr`, through a gate that tail-calls stock), the shared OFF/ON value-text routine at 0x400fb56c (prints PIPO for 2, stock text otherwise; the other OFF/ON parameters never reach 2), and LOP's maximum in stock's parameter table at 0x401c04f8 (data, 1.0 to 2.0). Loop Sync's audio hook at 0x40121020 runs earlier in the same interrupt; Pipo Loop swaps STA and END after it.
- A PIPO loop is a few frames shorter at each end than ON's (the turn comes up to one block early). Inaudible on most material, audible on very short loops.
- Stock plays a loop of at least 141 frames; Pipo Loop leaves loops shorter than 160 frames to stock's forward loop.
- A new trig always starts forward from STA, as stock does.
- On stock OS, PIPO plays as ON. Sounds and kits keep the value, so loading them again with the mod brings PIPO back.
- Not yet run on a unit.

## Tests and measurements

rytm-emu (c5732f0) on OS 1.74, build PP03, with a generated ramp sample on BD: the value steps OFF/ON/PIPO and reads PIPO; ON plays stock's forward loop and PIPO a click-free triangle at several pitches; p-locks and scene locks of PIPO; two voices at once; retrig; persistence through power-off; and stock 1.74 loading the saved PIPO state as a normal loop. CPU is a static estimate: about 0.2 % of an audio block with no voice in PIPO and about 0.7 % with all eight. See [TESTING.md](TESTING.md).

## Authorship and licences

Original code by npp1993 (Nathaniel Pavel), GPL-3.0-or-later ([LICENSE](LICENSE)). The OS 1.74 addresses were read by the author from his own stock firmware; the build carries addresses, lengths and SHA-256 guards only, no stock bytes. Screenshots: [media/LICENSE.md](media/LICENSE.md).

## Screens and audio

Screenshots are rytm-emu captures of build PP03 ([media/capture.json](media/capture.json)). No audio preview yet.
