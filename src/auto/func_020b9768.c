#include "m2c.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct Params {
    u8 near;
    u8 far;
    u8 pan_peak;
    u8 pan_end;
    u32 falloff;
} Params;

typedef struct Owner {
    u8 pad0[8];
    s32 flags;
    u8 pad1[0xb0 - 0xc];
    Vec3 pos;
} Owner;

typedef struct Emitter {
    Vec3 pos;
    Owner *owner;
    s32 last_dist;
    s32 handle;
    u16 sound;
    u8 flags;
    u8 active;
} Emitter;

typedef struct Manager {
    u8 pad0[0x38];
    Vec3 listener;
    Vec3 dir;
    Params *params;
} Manager;

extern Manager data_0210db30;

s32 func_020b8f70(u16 sound, s32 arg1);
void func_020b9068(s32 handle);
void func_020b973c(void *arg0);
void *func_020ba83c(s32 handle);
void func_020bae80(void *ch, s32 sample);
void func_020baedc(void *ch, s32 atten, s32 pan);
s32 _s32_div_f(s32 dividend, s32 divisor);

static inline void div32_set(s32 numer, u32 denom) {
    *(volatile u16 *) 0x4000280 = 0;
    *(volatile s32 *) 0x4000290 = numer;
    *(volatile u64 *) 0x4000298 = denom;
}

static inline void sqrt64_set(u64 param) {
    *(volatile u16 *) 0x40002b0 = 1;
    *(volatile u64 *) 0x40002b8 = param;
}

static inline void vec_sub(const Vec3 *a, const Vec3 *b, Vec3 *out) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

enum {
    HW_BUSY      = 0x8000,     /* DIVCNT / SQRTCNT busy bit */
    ATTEN_SILENT = 0xff,  /* beyond p->far: fully attenuated */
    ATTEN_STOP   = 0x40,    /* at or above: stop the voice, below: (re)start it */
    PAN_CENTER   = 0x20,
    PAN_MIN      = 0x10,
    PAN_MAX      = 0x70,
};

/* Per-frame update of a positional sound emitter (e = record built by
 * func_020b95f4, walked by func_020b9c90).
 *  1. Drop the owner link if the owner has flag 2 set; the source position is
 *     owner+0xb0 when linked, else the emitter's own Vec3.
 *  2. d = source - listener (manager +0x38); dist = sqrt(|d|^2) via the
 *     hardware square-root unit; numer = (d . manager+0x44) << 6 (fx32).
 *  3. Attenuation: 0 up to p->near, then ((dist - near) << 6) * falloff >> 36,
 *     ATTEN_SILENT at or beyond p->far.
 *  4. Pan (inside p->pan_end): hardware divide numer / dist, scaled by a ramp
 *     peaking at p->pan_peak, mapped to PAN_CENTER + r * 100 / 256 and clamped
 *     to [PAN_MIN, PAN_MAX]; PAN_CENTER otherwise. Distances are in units of
 *     the u8 params << 12.
 *  5. Start the voice (func_020b8f70) when attenuation falls below ATTEN_STOP,
 *     stop it (func_020b9068) and return when it rises to ATTEN_STOP.
 *  6. Look the voice up (func_020ba83c); if gone, release the emitter
 *     (func_020b973c) unless flag 2 is set. Else apply attenuation/pan
 *     (func_020baedc) and, with flag 1, feed the distance change since the
 *     last frame to func_020bae80; last_dist keeps this frame's distance. */
void func_020b9768(Emitter *e) {
    Params *p = data_0210db30.params;
    s32 pan;
    s32 atten = 0;
    Vec3 d; /* emitter position relative to the listener */
    Vec3 tmp;
    const Vec3 *src;
    s32 numer;
    u32 dist;
    s32 flag;
    void *ch;

    d.x = 0;
    d.y = 0;
    d.z = 0;
    if (e->owner != NULL && (e->owner->flags & 2) == 2) {
        e->owner = NULL;
    }
    if (e->owner != NULL) {
        src = &e->owner->pos;
    } else {
        src = &e->pos;
    }
    vec_sub(src, &data_0210db30.listener, &tmp);
    vec_set(&d, &tmp);

    sqrt64_set((s64) d.x * d.x + (s64) d.y * d.y + (s64) d.z * d.z);
    numer =
        (s32) (((s64) d.x * data_0210db30.dir.x + (s64) d.y * data_0210db30.dir.y + (s64) d.z * data_0210db30.dir.z + 0x800) >>
               12)
        << 6;
    while (*(volatile u16 *) 0x40002b0 & HW_BUSY) {
    }
    dist = *(volatile u32 *) 0x40002b4;

    if (dist >= p->far << 12) {
        atten = ATTEN_SILENT;
    } else {
        if (dist >= p->pan_end << 12) {
            flag = 1;
        } else {
            flag = 0;
            if (dist == 0) {
                div32_set(numer, 1);
            } else {
                div32_set(numer, dist);
            }
        }
        if (dist > p->near << 12) {
            s32 t = (dist - (p->near << 12)) << 6;
            atten = (s32) (((s64) t * p->falloff) >> 36);
        }
        if (flag == 0) {
            while (*(volatile u16 *) 0x4000280 & HW_BUSY) {
            }
            pan = *(volatile s32 *) 0x40002a0;
            if (dist < p->pan_peak << 12) {
                pan = _s32_div_f(dist * pan, p->pan_peak << 12);
            } else {
                pan = _s32_div_f(pan * ((p->pan_end << 12) - dist), (p->pan_end - p->pan_peak) << 12);
            }
            pan *= 100;
            pan >>= 8;
            pan += PAN_CENTER;
            if (pan < PAN_MIN) {
                pan = PAN_MIN;
            } else if (pan > PAN_MAX) {
                pan = PAN_MAX;
            }
        } else {
            pan = PAN_CENTER;
        }
    }

    if (e->active == 0) {
        if (atten < ATTEN_STOP) {
            e->active = 1;
            e->handle = func_020b8f70(e->sound, 0);
        }
    } else if (atten >= ATTEN_STOP) {
        e->active = 0;
        func_020b9068(e->handle);
        return;
    }
    ch = func_020ba83c(e->handle);
    if (ch == NULL) {
        if (!(e->flags & 2)) {
            func_020b973c(e);
        }
        return;
    }
    func_020baedc(ch, atten, pan);
    if (!(e->flags & 1)) {
        return;
    }
    if (e->last_dist == 0) {
        e->last_dist = dist;
        return;
    }
    func_020bae80(ch, e->last_dist - dist);
    e->last_dist = dist;
}
