// SPDX-License-Identifier: GPL-3.0-or-later
// Pipo Loop: zero snap. FUNC + turn on the SAMPLE page's STA or END moves
// that point to the next zero crossing of the sample, one crossing a detent,
// in the direction of the turn (audio.s hands the turn over).
//
// STA and END are 8.8 values of 120, a 1/30720 of the sample: the engine
// plays from frame frames * value / 30720. Up to 30720 frames every frame
// has a value, and the snap lands on the crossing's frame exactly; past it
// the nearest value is used, at most frames / 61440 frames from the
// crossing (half a step).
//
// A crossing is where the sample changes sign; samples within SILENT of zero
// count as neither sign, so silence and DC near zero are passed over, and
// the point lands on whichever sample of the pair is nearer zero. The search
// is bounded; with nothing in reach the turn stays stock's.

#include <stdint.h>

typedef unsigned long word_t;   // stock returns pointers in d0 (README trap)

#define SNAP_NONE 0x7fffffff
#define ID_SMP 41
#define ID_STA 43
#define ID_END 44
#define VT_PAGE_SET 0xa0        // page view: the parameter set of a track (-1: the selected one)
#define VT_READ 0x28            // set: read(set, id), 8.8
#define SLOT_DATA(s) (*(volatile short *volatile *)(SAMPLE_SLOTS_AT + (s) * 16))   // 16-bit mono
#define SLOT_FRAMES(s) (*(volatile unsigned *)(SAMPLE_SLOTS_AT + (s) * 16 + 8))
#define SLOT_LAST 127
#define FULL 30720              // 120.0 in 8.8
#define SILENT 8                // |sample| below this is no sign (about -72 dB)
#define REACH 262144            // frames searched a detent (about 5.5 s at 48 kHz)

static word_t vcall2(void *obj, int slot, int arg)
{
    word_t (*f)(void *, int) = (*(word_t (***)(void *, int))obj)[slot / 4];
    return f(obj, arg);
}

static int frame_of(unsigned frames, int v)
{
    return (int)((frames / FULL) * (unsigned)v + ((frames % FULL) * (unsigned)v) / FULL);
}

static int value_of(unsigned frames, int f)   // the value whose frame is f, or the nearest
{
    int lo = 0, hi = FULL;                     // the first value whose frame is f or after it
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (frame_of(frames, mid) < f)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo > 0 && frame_of(frames, lo) != f && f - frame_of(frames, lo - 1) < frame_of(frames, lo) - f)
        lo--;                                  // past 30720 frames: the nearer of the two
    return lo;
}

static int sign_of(int x)
{
    return x > SILENT ? 1 : x < -SILENT ? -1 : 0;
}

int pipo_loop_snap(void *view, int id, int delta)
{
    void *set = (void *)vcall2(view, VT_PAGE_SET, -1);
    volatile short *data;
    unsigned frames;
    int cur, other, slot, dir, f, lim, s, i, n, v;
    if (!set || !delta)
        return SNAP_NONE;
    cur = (int)vcall2(set, VT_READ, id);
    other = (int)vcall2(set, VT_READ, id == ID_STA ? ID_END : ID_STA);
    slot = (int)vcall2(set, VT_READ, ID_SMP) >> 8;
    if (slot <= 0 || slot > SLOT_LAST)
        return SNAP_NONE;
    data = SLOT_DATA(slot);
    frames = SLOT_FRAMES(slot);
    if (!data || frames < 2)
        return SNAP_NONE;
    dir = delta > 0 ? 1 : -1;
    f = frame_of(frames, cur);
    lim = dir > 0 ? (int)frames - 1 : 0;   // the sample's edge, or the other point when it lies that way
    if (dir > 0 ? other > cur : other < cur) {
        int o = frame_of(frames, other) - dir;
        if (dir > 0 ? o < lim : o > lim)
            lim = o;
    }
    if (f > (int)frames - 1)
        f = (int)frames - 1;
    s = sign_of(data[f]);
    for (i = f + dir, n = 0; (dir > 0 ? i <= lim : i >= lim) && n < REACH; i += dir, n++) {
        int t = sign_of(data[i]);
        if (!t)
            continue;
        if (s && t != s) {           // the sign changed between i - dir and i
            int a = data[i] < 0 ? -data[i] : data[i];
            int p = i - dir, b = data[p] < 0 ? -data[p] : data[p];
            int c = b < a && p != f ? p : i;
            v = value_of(frames, c);
            if (dir > 0 ? v > cur : v < cur)
                return v - cur;
        }
        s = t;
    }
    return SNAP_NONE;
}
