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

| Stock's OFF/ON switch icon, icon(ctx, value 8.8, ...), draws frame 0 or 1
| of a two-frame switch; BD CLASSIC's WAV (0..2) is drawn by the same routine
| with a three-frame switch. For LOP's record only (ctx is its record + 36),
| the gate tail-jumps to WAV's routine with the arguments untouched, so LOP's
| OFF, ON and PIPO show as stock's three-position switch. Every other OFF/ON
| switch replays the displaced load of its frame list and resumes stock.

        .globl  pipo_loop_icon_gate

pipo_loop_icon_gate:
        lea     pipo_loop_icon_ctx,%a0
        cmpa.l  4(%sp),%a0
        bne.s   1f
        jmp     pipo_loop_icon3
1:      movea.l pipo_loop_icon_frames,%a0       | displaced
        jmp     pipo_loop_icon_resume

| Zero snap: FUNC + turn on the SAMPLE page's STA or END. Stock's encoder
| handler ends by calling the view's apply-delta with FUNC held, which steps
| STA and END by whole numbers. The site replaces the first two instructions
| of that call's setup (d6 = FUNC held, d2 the id, d4 the delta, a2 the view,
| a0 the view's vtable). For STA and END with FUNC held, pipo_loop_snap gives
| the delta to the next zero crossing the way the turn went; the call then
| goes out as an ordinary fine turn of that size, so stock's own path clamps,
| draws and stores it. With no crossing in reach it returns SNAP_NONE and the
| turn stays stock's whole-number step.

        .equ    SNAP_NONE, 0x7fffffff
        .globl  pipo_loop_func_gate
        .extern pipo_loop_snap

pipo_loop_func_gate:
        mvz.b   %d6,%d6                         | displaced
        tst.l   %d6
        beq.s   9f
        moveq   #43,%d0                         | STA
        cmp.l   %d2,%d0
        beq.s   1f
        moveq   #44,%d0                         | END
        cmp.l   %d2,%d0
        bne.s   9f
1:      move.l  %d4,-(%sp)
        move.l  %d2,-(%sp)
        move.l  %a2,-(%sp)
        jsr     pipo_loop_snap
        lea     12(%sp),%sp
        movea.l (%a2),%a0                       | the view's vtable, as stock had it
        cmp.l   #SNAP_NONE,%d0
        beq.s   9f
        move.l  %d0,%d4                         | the step to the crossing
        moveq   #0,%d6                          | applied as a fine turn
9:      pea     -34(%fp)                        | displaced
        jmp     pipo_loop_func_resume

        .section .rodata
pipo_loop_pipo:
        .asciz  "PIPO"
