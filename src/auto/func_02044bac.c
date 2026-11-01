#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

extern M2C_UNK data_0210930c;
extern Vec3 data_020ff9b4;

typedef struct {
    s32 v;
} Word;

typedef struct {
    u8 pad0[0x1C];
    void *cur;
    u8 pad20[4];
    void *tbl;
} Sub;
typedef struct {
    u8 pad0[0x5E0];
    Sub sub;
} Obj;
M2C_UNK func_0201eec4(void *, void *, Word);
s32 func_0202c59c(void *, s32, s32);
s32 func_0202cb44(void *);
void func_0202cbd4(s32 *, void *);
void func_0202cbf4(s32 *, const void *);
void func_0204a62c(void *, void *);
s32 FX_Div(Word, s32); // FAKE: numerator typed as one-word struct; by-value load gives add r0,sp,#0; ldr r0,[r0]
void func_020675d8(void *, s32);
void func_02004b68(void *, s32, void *, s32);

void func_02044bac(void *arg0) {
    Word w;
    s32 result = 0;
    s32 flag   = 0;
    s32 a, b, c, d;
    Word e;
    s32 thr;
    s32 big;
    s32 type;
    s32 t;
    void *p;
    void *q;
    Obj *o;

    p = M2C_FIELD(arg0, void **, 4);
    if (M2C_FIELD(p, u8 *, 0x698) != 0) {
        w.v = 0x8000;
        if (func_0201eec4((s8 *) p + 0xB0, (s8 *) arg0 + 0x18, w) != 0) {
            flag = 1;
        }
    }
    if (flag != 0) {
        result = 1;
    } else {
        p = M2C_FIELD(arg0, void **, 4);
        if (M2C_FIELD(p, u8 *, 0x696) == 0) {
            result = 1;
        } else if (M2C_FIELD(arg0, u8 *, 0x24) != 0 && M2C_FIELD(p, s32 *, 0x5FC) != 0) {
            if (func_0202cb44((s8 *) p + 0x5E0) != 0) {
                o    = M2C_FIELD(arg0, Obj **, 4);
                a    = 0;
                q    = o->sub.cur;
                thr  = M2C_FIELD(o->sub.tbl, s32 *, 8);
                type = M2C_FIELD(q, u8 *, 9);
                if ((u32) (type - 4) <= 0x10 && ((1 << (type - 4)) & 0x18001)) {
                    a = 1;
                }
                b = (M2C_FIELD(M2C_FIELD(o, void **, 0x5FC), u8 *, 8) == 4);
                func_0202cbd4(&c, M2C_FIELD(o, void **, 0x5FC));
                if (thr <= c) {
                    if (a != 0) {
                        result = 1;
                    }
                }
                if (result == 0 && b == 0) {
                    func_0202cbf4(&d, M2C_FIELD(M2C_FIELD(arg0, void **, 4), void **, 0x5FC));
                    big = thr < d;
                    t   = func_0202c59c((s8 *) M2C_FIELD(arg0, void **, 4) + 0x5E0, (s32) ((s8 *) arg0 + 0x10),
                                        (s32) ((s8 *) arg0 + 0x14));
                    if (big) {
                        if (t != 0) {
                            if (a != 0) {
                                result = 1;
                            }
                        } else if (a != 0) {
                            func_0202cbf4(&e.v, M2C_FIELD(M2C_FIELD(arg0, void **, 4), void **, 0x5FC));
                            if (FX_Div(e, 0x2000) - thr > 0) { // FAKE: struct-typed numerator, see FX_Div decl
                                result = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    if (result != 0) {
        if (M2C_FIELD(arg0, u8 *, 0x25) != 0) {
            func_020675d8((s8 *) M2C_FIELD(M2C_FIELD(M2C_FIELD(&data_0210930c, void **, 0x68), void **, 0x38C), s8 **, 4) +
                              0x28,
                          M2C_FIELD(arg0, u8 *, 0x25));
        }
        p = M2C_FIELD(arg0, void **, 4);
        M2C_FIELD(p, u32 *, 8) |= 0x200;
        {
            Vec3 *dst = (Vec3 *) ((s8 *) M2C_FIELD(arg0, void **, 4) + 0x60);
            if (dst != &data_020ff9b4) {
                dst->x = data_020ff9b4.x;
                dst->y = data_020ff9b4.y;
                dst->z = data_020ff9b4.z;
            }
        }
        p = M2C_FIELD(arg0, void **, 4);
        func_02004b68(p, 1, p, 2);
        M2C_FIELD(arg0, u8 *, 0xC) |= 1;
    } else {
        p = M2C_FIELD(arg0, void **, 4);
        func_0204a62c((s8 *) p + 0x5CE, p);
    }
}
