#include "m2c.h"

typedef struct {
    u16 v;
} Ang;

typedef struct {
    Ang v;
    u8 f2;
    u8 f3;
    u8 f4;
    u8 f5;
} Key;

typedef struct {
    u8 pad0[0xb0];
    u8 fb0[0x1a];
    u16 fca;
    u8 pad_cc[0x6b0 - 0xcc];
    u8 *f6b0;
} Target;

typedef struct {
    u8 pad0[4];
    Target *f4;
    u8 pad8[0x16];
    u8 f1e;
} Obj;

extern void func_0201807c(Ang *, Target *, void *, s32);
extern void func_0202732c(Key *);
extern void func_02027470(Key *, Target *, Ang);
extern void func_02027348(Key *, Key *);
extern void func_020278ac(Key *, Target *);
extern void func_02027388(Key *, Key *, u8 *, s32);
extern void func_02030038(Obj *);
extern void func_02030540(Obj *, u8, Ang);
extern s32 func_0203180c(Obj *, s32, s32, s32, Ang, Ang);
extern s32 func_02034978(Obj *);
extern void func_02035720(Obj *, s32);
extern void func_020357c8(Obj *);
extern s32 func_02062a7c(s32, s32);
extern s32 func_020835fc(void *, void *);
extern const u8 data_020ed284[];

enum {
    ANG_60_DEG    = 0x2aaa, /* 0x10000 = full turn */
    JITTER_10_DEG = 1820,
    F1E_BIT1      = 2,
};

static inline Ang ang_make(u16 v) {
    Ang a;
    a.v = v;
    return a;
}

static inline u16 ang_get(Ang a) {
    return a.v;
}

static inline u16 ang_val(const Ang *p) {
    return ang_get(*p);
}

static inline Ang ang_add(u16 base, const Ang *d) {
    Ang a;
    a.v = base + ang_val(d);
    return a;
}

static inline u16 ang_to(Target *t, void *pos) {
    Ang a;
    func_0201807c(&a, t, pos, 0);
    return ang_val(&a);
}

static inline u16 ang_jitter(Obj *this) {
    Ang a;
    a.v = this->f4->fca + (u16) func_02062a7c(-JITTER_10_DEG, JITTER_10_DEG);
    return ang_val(&a);
}

/*
 * func_0203180c is tried first (args 3, 0x96, 0, angles 0 and 60 deg); a
 * nonzero result goes to func_02035720, then func_020357c8, and we return.
 * Otherwise func_02034978 runs; if f1e bit 1 is set, func_02030038 runs and we
 * return. Else three keys are initialised and an angle h is chosen: from
 * func_0201807c(f4, f4->f6b0 + 4) when f6b0 is set and
 * func_020835fc(f6b0, f4->fb0) returns 0, else f4->fca plus
 * func_02062a7c(-1820, 1820) (about +-10 deg). Keys built from h, from f4 and
 * from data_020ed284[6..7] are merged, and func_02030540 gets the merged key's
 * f2 byte and f4->fca plus its angle.
 */
void func_02034c68(Obj *this) {
    Key k3a;
    Key k34;
    Key k2e;
    Key k28;
    Key k22;
    Key k1c;
    u8 w[2];
    Ang h;
    s32 r;

    r = func_0203180c(this, 3, 0x96, 0, ang_make(0), ang_make(ANG_60_DEG));
    if (r != 0) {
        func_02035720(this, r);
        func_020357c8(this);
        return;
    }
    func_02034978(this);
    if (this->f1e & F1E_BIT1) {
        func_02030038(this);
        return;
    }
    w[1] = data_020ed284[7];
    w[0] = data_020ed284[6];
    func_0202732c(&k34);
    func_0202732c(&k3a);
    func_0202732c(&k2e);
    if (this->f4->f6b0 != 0 && func_020835fc(this->f4->f6b0, this->f4->fb0) == 0) {
        h.v = ang_to(this->f4, this->f4->f6b0 + 4);
    } else {
        h.v = ang_jitter(this);
    }
    func_02027470(&k28, this->f4, h);
    func_02027348(&k34, &k28);
    func_020278ac(&k22, this->f4);
    func_02027348(&k3a, &k22);
    func_02027388(&k1c, &k34, w, 2);
    func_02027348(&k2e, &k1c);
    func_02030540(this, k2e.f2, ang_add(this->f4->fca, &k2e.v));
}
