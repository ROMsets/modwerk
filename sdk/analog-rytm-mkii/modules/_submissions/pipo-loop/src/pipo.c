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
// effective parameter array. A new trig starts forward again.
//
// The swap does not rely on stock rebuilding the array every block: when the
// values still read as the ones written a block ago, they are put back first.
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
#define TRIG_NEXT (*(volatile unsigned *)TRIG_NEXT_AT)   // one bit a voice, as stock's prefetch tests it

#define LOP_PIPO 2          // LOP's third value; stock reads any nonzero LOP as ON
#define VOICES 8
#define TRACKS 12
#define MIN_SPAN 160        // stock plays a loop of at least 141 frames
#define GUARD 8             // frames kept clear of the far end for the interpolator
#define MAX_STEP 512        // a larger move between blocks is a jump, not travel

struct voice {
    int last_pos, step;          // position a block ago; frames travelled a block
    int lo, hi;                  // the loop in frames, for c_sta, c_end and c_frames
    unsigned c_frames;
    short c_sta, c_end;
    short w_sta, w_end;          // what this handler wrote, while wrote is set
    unsigned char fwd;           // STA below END
    unsigned char up;            // the voice moves towards higher frames this block
    unsigned char back;          // playing the way back: STA and END swapped
    unsigned char seen, wrote, track;
};

static struct voice voices[VOICES];

static int frame_of(unsigned frames, int v8)   // STA/END (8.8, of 120) to stock's frame: ceil(frames * v8 / 30720)
{
    if (v8 < 0)
        v8 = 0;
    return (int)((frames / 30720u) * (unsigned)v8 + ((frames % 30720u) * (unsigned)v8 + 30719u) / 30720u);
}

void pipo_loop_audio(void)
{
    unsigned trig = TRIG_NEXT;
    int v;
    for (v = 0; v < VOICES; v++) {
        struct voice *p = &voices[v];
        int t = OWNER(v), sta, end, pos, d, ahead;
        unsigned frames;
        if (p->wrote) {                  // still as written: stock did not rebuild it
            if (EFF(p->track, IDX_STA) == p->w_sta && EFF(p->track, IDX_END) == p->w_end) {
                EFF(p->track, IDX_STA) = p->w_end;
                EFF(p->track, IDX_END) = p->w_sta;
            }
            p->wrote = 0;
        }
        if (t >= TRACKS || !(EFF(t, IDX_LOP) >> 8)) {   // no voice, or LOP OFF: nothing to follow
            p->back = 0;
            p->seen = 0;
            continue;
        }
        pos = VPOS(v);
        if (p->seen) {                   // travel only in the way it was moving
            d = pos - p->last_pos;
            if (p->up ? d > 0 && d <= MAX_STEP : d < 0 && d >= -MAX_STEP)
                p->step = d < 0 ? -d : d;
        }
        p->last_pos = pos;
        p->seen = 1;
        sta = EFF(t, IDX_STA);
        end = EFF(t, IDX_END);
        frames = VFRAMES(v);
        if (sta != p->c_sta || end != p->c_end || frames != p->c_frames) {
            int a = frame_of(frames, sta), b = frame_of(frames, end);
            p->lo = a < b ? a : b;
            p->hi = a < b ? b : a;
            if (p->hi > (int)frames)
                p->hi = (int)frames;
            p->fwd = a < b;
            p->c_sta = sta;
            p->c_end = end;
            p->c_frames = frames;
        }
        if ((trig & (1u << v))                       // restarts from STA this block
            || (EFF(t, IDX_LOP) >> 8) != LOP_PIPO     // OFF, or ON: stock's loop
            || p->hi - p->lo < MIN_SPAN)
            p->back = 0;
        else {
            ahead = p->step + GUARD;     // this block's travel
            if (p->fwd ^ p->back ? pos + ahead >= p->hi : pos - ahead <= p->lo)
                p->back ^= 1;
        }
        p->up = p->fwd ^ p->back;
        if (p->back) {
            EFF(t, IDX_STA) = (short)end;
            EFF(t, IDX_END) = (short)sta;
            p->w_sta = (short)end;
            p->w_end = (short)sta;
            p->wrote = 1;
            p->track = (unsigned char)t;
        }
    }
}
