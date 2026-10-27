| SPDX-License-Identifier: GPL-3.0-or-later
| Pipo Loop: the audio interrupt's hook and LOP's PIPO text.
|
| The audio interrupt calls the sample prefetch for voice 0 once a block,
| after the effective parameters (locks, scenes, LFO) are complete and
| before any voice reads STA, END and LOP. The site is that `jsr`; the gate
| runs pipo_loop_audio and then tail-jumps to the stock routine with the
| stack as the caller left it (return address and its two arguments), so the
| stock routine returns straight to the interrupt. d0-d1/a0-a1 are free
| across a call; the C handler keeps the rest.

        .text
        .globl  pipo_loop_gate
        .extern pipo_loop_audio

pipo_loop_gate:
        jsr     pipo_loop_audio
        jmp     pipo_loop_prefetch

| Stock's OFF/ON text formatter, text(ctx, value 8.8, buf), shared by eight
| OFF/ON parameters; ctx is the parameter's record + 20. The site replaces
| its first two instructions, which take value's integer part into d0; the
| gate does the same, prints PIPO for 2 on LOP's record only (MIDI CC can put
| 2 in the others, which stay ON) and otherwise resumes stock, which reloads
| d1 itself. PIPO goes through stock's own sprintf(buf, text) tail call, as
| OFF and ON do.

        .globl  pipo_loop_text_gate

pipo_loop_text_gate:
        move.l  8(%sp),%d0                      | displaced
        asr.l   #8,%d0                          | displaced
        moveq   #2,%d1
        cmp.l   %d1,%d0
        bne.s   2f
        lea     pipo_loop_lop_ctx,%a0
        cmpa.l  4(%sp),%a0
        beq.s   1f
2:      jmp     pipo_loop_text_resume
1:      move.l  12(%sp),4(%sp)                  | buf
        lea     pipo_loop_pipo,%a0              | the text (ColdFire: no #imm32 to d16(An))
        move.l  %a0,8(%sp)
        jmp     pipo_loop_sprintf

        .section .rodata
pipo_loop_pipo:
        .asciz  "PIPO"
