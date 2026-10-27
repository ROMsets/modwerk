| SPDX-License-Identifier: GPL-3.0-or-later
| Pipo Loop: the audio interrupt's hook.
|
| The audio interrupt calls the sample prefetch for voice 0 once a block,
| after the effective parameters (locks, scenes, LFO) are complete and
| before any voice reads STA, END and LOP. The site is that `jsr`; the gate
| runs pipo_audio and then tail-jumps to the stock routine with the stack
| as the caller left it (return address and its two arguments), so the
| stock routine returns straight to the interrupt. d0-d1/a0-a1 are free
| across a call; the C handler keeps the rest.

        .text
        .globl  pipo_loop_gate
        .extern pipo_loop_audio

pipo_loop_gate:
        jsr     pipo_loop_audio
        jmp     pipo_loop_prefetch
