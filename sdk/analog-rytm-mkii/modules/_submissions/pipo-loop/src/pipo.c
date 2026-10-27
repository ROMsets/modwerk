// SPDX-License-Identifier: GPL-3.0-or-later
// Pipo Loop for the Analog Rytm MKII: ping-pong sample loops.
//
// LOP gets a third value, PIPO (OFF, ON, PIPO): a sample whose effective LOP
// is PIPO plays its loop forward, then backward, and so on. ON stays stock's
// forward loop. The range and the text are build.json's sites; stock reads
// any nonzero LOP as a loop, so a sound saved with PIPO plays as ON on an OS
// without this mod.
//
// The stock sample engine reads each track's effective STA, END and LOP once
// an audio block and plays from STA towards END, backward when END is below
// STA; the play position is an absolute frame index either way. So swapping
// the effective STA and END turns a voice around where it is. Each block,
// before the engine reads them, this handler looks at every voice's position:
// when the coming block would reach the loop's far end, it flips that voice's
// direction, and while a voice plays its way back it swaps STA and END in the
// effective parameter array (rebuilt by stock every block, so the swap never
// accumulates). A new trig starts forward again.
//
// The turn falls on a block boundary, so it comes up to one block (32 frames
// at unity pitch) plus GUARD before the loop's end. Looking one step ahead is
// the least that works: with none, stock wraps the voice to STA first.

#include <stdint.h>

#define EFF(t, i) (*(volatile short *)(PARAMS_EFF_AT + (t) * 84 + (i) * 2))
#define IDX_STA 21
#define IDX_END 22
#define IDX_LOP 23
#define VREC(v) (VOICE_REC_AT + (v) * 92)
#define VPOS(v) (*(volatile int *)(VREC(v) + 4))
#define VFRAMES(v) (*(volatile unsigned *)(VREC(v) + 20))
#define OWNER(v) (*(volatile unsigned char *)(VOICE_OWNER_AT + (v)))
#define TRIG_NEXT (*(volatile unsigned *)TRIG_NEXT_AT)

#define LOP_PIPO 2          // LOP's third value; stock reads any nonzero LOP as ON
#define VOICES 8
#define TRACKS 12
#define MIN_SPAN 160        // stock plays a loop of at least 141 frames
#define GUARD 8             // frames kept clear of the far end for the interpolator

static unsigned char back[VOICES];   // 1: playing the way back (STA and END swapped)
static int last_pos[VOICES];
static unsigned char seen[VOICES];

static int frame_of(unsigned frames, int v8)   // STA/END (8.8, of 120) to a frame
{
    if (v8 < 0)
        v8 = 0;
    return (int)((frames / 30720u) * (unsigned)v8 + ((frames % 30720u) * (unsigned)v8) / 30720u);
}

void pipo_loop_audio(void)
{
    unsigned trig = TRIG_NEXT;
    int v;
    for (v = 0; v < VOICES; v++) {
        int t = OWNER(v), sta, end, a, b, lo, hi, pos, step, ahead, fwd;
        unsigned frames;
        if (t >= TRACKS)
            continue;
        if (trig & (1u << v)) {          // retriggers this block: from STA, forward
            back[v] = 0;
            seen[v] = 0;
            continue;
        }
        if ((EFF(t, IDX_LOP) >> 8) != LOP_PIPO) {   // OFF, or ON: stock's loop
            back[v] = 0;
            seen[v] = 0;
            continue;
        }
        sta = EFF(t, IDX_STA);
        end = EFF(t, IDX_END);
        frames = VFRAMES(v);
        a = frame_of(frames, sta);
        b = frame_of(frames, end);
        lo = a < b ? a : b;
        hi = a < b ? b : a;
        if (hi > (int)frames)
            hi = (int)frames;
        if (hi - lo < MIN_SPAN) {
            back[v] = 0;
            seen[v] = 0;
            continue;
        }
        pos = VPOS(v);
        step = seen[v] ? pos - last_pos[v] : 0;
        if (step < 0)
            step = -step;
        if (step > 512)                  // a wrap or a jump: not a step
            step = 0;
        last_pos[v] = pos;
        seen[v] = 1;
        ahead = step + GUARD;            // this block's travel
        fwd = (a < b) ^ back[v];         // the way the voice plays now
        if (fwd ? pos + ahead >= hi : pos - ahead <= lo)
            back[v] ^= 1;
        if (back[v]) {
            EFF(t, IDX_STA) = (short)end;
            EFF(t, IDX_END) = (short)sta;
        }
    }
}
