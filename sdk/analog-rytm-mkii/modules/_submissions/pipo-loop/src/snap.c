// SPDX-License-Identifier: GPL-3.0-or-later
// Pipo Loop: zero snap. FUNC + turn on the SAMPLE page's STA or END moves
// that point to the next zero crossing of the sample in the direction of the
// turn, one crossing for each detent's worth of encoder counts (COUNTS, as
// pipo_loop_enc counts them, at most one an encoder event), on the sound or on held trigs' locks (audio.s
// hands the turn over).
//
// STA and END are 8.8 values of 120, 1/30720 of the sample: stock's prefetch
// plays from frame ceil(frames * value / 30720) (its EMAC multiply by 8/15,
// worked through for every value). Up to 30720 frames every frame has a
// value, and the snap lands on the crossing's frame exactly; past it the
// nearest value is used, at most frames / 61440 frames from the crossing.
//
// A crossing is where the sample changes sign; samples within SILENT of zero
// count as neither sign, so silence and DC near zero are passed over. The
// point lands on the first sample of the new sign, for STA and END alike, so
// a loop's wrap from END - 1 to STA meets the sample as a crossing does.
// With LOP ON, a point steps only through crossings of the same direction as
// the other point's, when that one sits on a crossing: the forward loop's
// wrap then joins two crossings of one slope. The search is bounded; with
// nothing in reach the turn stays stock's.

#include <stdint.h>

typedef unsigned long word_t;   // stock returns pointers in d0 (README trap)

#define SNAP_NONE 0x7fffffff
#define ID_SMP 41
#define ID_STA 43
#define ID_END 44
#define ID_LOP 45
#define VT_PAGE_SET 0xa0        // page view: the parameter set of a track (-1: the selected one)
#define VT_READ 0x28            // set: read(set, id), 8.8
#define VT_VALUE 0x54           // page view: value(view, id, ctx), 8.8 (ctx -1: the sound's)
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

static word_t vcall3(void *obj, int slot, int a, int b)
{
    word_t (*f)(void *, int, int) = (*(word_t (***)(void *, int, int))obj)[slot / 4];
    return f(obj, a, b);
}

static int frame_of(unsigned frames, int v)   // stock's start or end frame for a value
{
    return (int)((frames / FULL) * (unsigned)v + ((frames % FULL) * (unsigned)v + FULL - 1) / FULL);
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

// lock: the turn edits a held trig's lock through the view's lock editor
// (LOCK_EDITOR of view + 108), as stock's lock edit does when there is one:
// the trig's lock if it has one (editor vt + 28, read through vt + 24), else
// the view's value (the sound's). Without an editor stock hands the turn to
// each held trig in turn (pipo_loop_snap_trig).
typedef word_t (*fn1_t)(word_t);
typedef unsigned (*is_key_held_t)(int key);
#define LOCK_EDITOR ((fn1_t)LOCK_EDITOR_AT)
#define TRACK_OF ((fn1_t)TRACK_INDEX_OF_AT)
#define LOCK_SLOT ((fn1_t)LOCK_SLOT_AT)
#define IS_KEY_HELD ((is_key_held_t)IS_KEY_HELD_AT)
#define KEY_FUNC 0x80
#define VT_HAS_LOCK 0x1c        // lock editor: has(ed, track, slot)
#define VT_LOCK 0x18            // lock editor: lock(ed, track, slot), 8.8
#define VT_TRIG_LOCK 0x70       // page view: lock(view, trig, id), 8.8, below 0 when none
#define VT_TRIG_CTX 0x84        // page view: the trig's context for VT_VALUE

// +1 a rising crossing (the sample goes from below zero to above), -1 a
// falling one, 0 none: at frame f, against the last signed sample before it.
static int crossing_at(volatile short *data, int frames, int f)
{
    int t, i;
    if (f <= 0 || f >= frames || !(t = sign_of(data[f])))
        return 0;
    for (i = f - 1; i >= 0 && f - i < 64; i--) {
        int u = sign_of(data[i]);
        if (u)
            return u != t ? t : 0;
    }
    return 0;
}

// The step from cur to the steps-th next zero crossing the way steps goes,
// or SNAP_NONE. lop: LOP's integer part (1, ON: match the other point's
// crossing direction).
static int snap_from(int cur, int other, int slot, int steps, int lop)
{
    volatile short *data;
    unsigned frames;
    int dir = steps > 0 ? 1 : -1, f, lim, s, i, k, n, v = cur, want = 0;
    if (slot <= 0 || slot > SLOT_LAST || !steps)
        return SNAP_NONE;
    data = SLOT_DATA(slot);
    frames = SLOT_FRAMES(slot);
    if (!data || frames < 2)
        return SNAP_NONE;
    if (lop == 1)
        want = crossing_at(data, (int)frames, frame_of(frames, other));
    lim = dir > 0 ? (int)frames - 1 : 1;   // the sample's edge, or the other point when it lies that way
    if (dir > 0 ? other > cur : other < cur) {
        int o = frame_of(frames, other) - dir;
        if (dir > 0 ? o < lim : o > lim)
            lim = o;
    }
    for (; steps; steps -= dir) {
        f = frame_of(frames, v);
        if (f > (int)frames - 1)
            f = (int)frames - 1;
        // Scan from f the way the turn goes for the next sign change; the
        // crossing's frame is the first sample of the new sign.
        s = sign_of(data[f]);
        k = f;                           // the last signed sample the scan passed
        for (i = f + dir, n = 0; ; i += dir, n++) {
            int t;
            if ((dir > 0 ? i > lim : i < lim) || n >= REACH)
                return v != cur ? v - cur : SNAP_NONE;
            t = sign_of(data[i]);
            if (!t)
                continue;
            if (s && t != s) {
                int c = dir > 0 ? i : k;           // the first signed sample of the new sign, in time
                int w = value_of(frames, c);
                int kind = dir > 0 ? t : s;          // the new sign, in time: +1 rising
                if ((!want || kind == want) && (dir > 0 ? w > v : w < v)) {
                    v = w;
                    break;
                }
            }
            s = t;
            k = i;
        }
    }
    return v - cur;
}

static int editor_value(void *ed, int track, int id, void *view)
{
    int slot = (int)LOCK_SLOT((word_t)id);
    if ((unsigned char)vcall3(ed, VT_HAS_LOCK, track, slot))
        return (int)(short)vcall3(ed, VT_LOCK, track, slot);
    return (int)(short)vcall3(view, VT_VALUE, id, -1);
}

// Encoder counts, from core-ar2's ev_enc: FUNC + E or F on any page (only
// STA and END's turns use them). One crossing for each COUNTS counts (a
// detent's, as stock's encoder sends them), the rest kept for the next event;
// each event's steps are worked out once, for every path stock takes it by
// (one held trig after another).
#define COUNTS 1            // a slow detent on a unit sends 1 count (PD01: 1-3 counts, as many events)
#define ENC_STA 4
#define ENC_END 5
static int enc_acc, enc_last = -1, enc_serial, used_serial = -1, used_steps;

int pipo_loop_enc(void *brain, char *ev)
{
    int enc = *(int *)(ev + 12), count = *(int *)(ev + 16);
    (void)brain;
    if ((enc != ENC_STA && enc != ENC_END) || !IS_KEY_HELD(KEY_FUNC)) {
        enc_acc = 0;
        enc_last = -1;
        return 0;
    }
    if (enc != enc_last || (count > 0) != (enc_acc > 0))
        enc_acc = 0;                 // another knob or the other way: start the count again
    enc_last = enc;
    enc_acc += count;
    enc_serial++;
    return 0;                        // stock goes on with the turn
}

static int steps_now(void)
{
    if (used_serial != enc_serial) {
        // One crossing an encoder event, whatever its count: on a unit a
        // slow detent sends 1 count, and a fast turn sends events of 15 or
        // more (stock's acceleration, PD01). What is left over is dropped.
        used_steps = enc_acc >= COUNTS ? 1 : enc_acc <= -COUNTS ? -1 : 0;
        if (used_steps)
            enc_acc = 0;
        used_serial = enc_serial;
    }
    return used_steps;
}

// From audio.s, FUNC + turn on STA or END: the sound's value (lock 0), or a
// held trig's lock through the lock editor (lock 1).
int pipo_loop_snap(void *view, int id, int delta, int lock)
{
    int other = id == ID_STA ? ID_END : ID_STA, steps = steps_now();
    if (!delta)
        return SNAP_NONE;
    if (!steps)
        return 0;                    // less than a detent yet: hold the point
    if (lock) {
        void *ed = (void *)LOCK_EDITOR(*(word_t *)((char *)view + 108));
        int track;
        if (!ed)
            return SNAP_NONE;        // stock goes trig by trig: pipo_loop_snap_trig
        track = (int)TRACK_OF(*(word_t *)((char *)view + 116));
        return snap_from(editor_value(ed, track, id, view), editor_value(ed, track, other, view),
                         editor_value(ed, track, ID_SMP, view) >> 8, steps, editor_value(ed, track, ID_LOP, view) >> 8);
    } else {
        void *set = (void *)vcall2(view, VT_PAGE_SET, -1);
        if (!set)
            return SNAP_NONE;
        return snap_from((int)vcall2(set, VT_READ, id), (int)vcall2(set, VT_READ, other),
                         (int)vcall2(set, VT_READ, ID_SMP) >> 8, steps, (int)vcall2(set, VT_READ, ID_LOP) >> 8);
    }
}

static int trig_value(void *view, int trig, int id)
{
    int v = (int)vcall3(view, VT_TRIG_LOCK, trig, id);
    if (v >= 0)
        return (int)(short)v;
    return (int)(short)vcall3(view, VT_VALUE, id, (int)vcall2(view, VT_TRIG_CTX, trig));
}

// From audio.s, stock's per-trig lock edit, for a trig that has a lock: with
// FUNC held on STA or END, the step from that trig's own lock.
int pipo_loop_snap_trig(void *view, int trig, int id, int delta)
{
    int lock, steps;
    if (!delta || (id != ID_STA && id != ID_END) || !IS_KEY_HELD(KEY_FUNC))
        return SNAP_NONE;
    lock = (int)vcall3(view, VT_TRIG_LOCK, trig, id);
    if (lock < 0)
        return SNAP_NONE;
    if (!(steps = steps_now()))
        return 0;
    return snap_from((int)(short)lock, trig_value(view, trig, id == ID_STA ? ID_END : ID_STA),
                     trig_value(view, trig, ID_SMP) >> 8, steps, trig_value(view, trig, ID_LOP) >> 8);
}
