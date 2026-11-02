#include "m2c.h"

class CObj {
public:
    virtual void f0();
    virtual void f1();
    virtual void f2();
    virtual void f3();
    virtual void f4();
    virtual void f5();
    virtual void f6();
    virtual void f7();
    virtual void f8();
    virtual void f9(s32 mode, s32 a, s32 b, s32 c);
    virtual void f10();
    virtual void f11();
    virtual void f12(u16 w, s32 flag);
    virtual void f13(s32 flag);
};

typedef struct {
    u8 b[4];
} Bytes4;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

extern "C" {
extern void *func_0200c04c(void *self, s32 a1, s32 a2, s32 a3, void *a4);
extern void *func_0204a18c(void *arg0);
extern void *func_0202be6c(void *arg0);
extern u8 func_020917b4(void *arg0);
extern u8 func_02093fa8(s32 arg0);
extern void func_02006b24(void *object);
extern void func_02001990(void *self, s32 mode);
extern void func_02004c80(void);
extern u16 func_0201ca1c(u16 arg0);
extern s32 func_020ab40c(void *arg0, u16 arg1);
extern void func_0201c778(u16 *values);
extern void func_020063d4(void *self, s32 id, u16 *values);
extern void func_02004404(void *self, u32 arg1);
extern s32 func_020062b4(void *self);
extern s32 func_0200631c(void *self);
extern s32 func_0208db80(u32 arg0);
extern u32 func_02062ae4(s32 arg0);
extern void func_02003784(void *self, u16 kind, u16 *values);
extern void func_02006220(void *self, u16 *values);
extern void func_0201c858(u16 *arg0, s32 arg1, u16 arg2);
extern s32 func_020ab574(void *arg0, u16 *arg1, s32 arg2);
extern void func_0200c85c(void *self, s32 arg1);
extern void func_02003ac4(void *self);
extern void func_02003cb8(void *self);
extern void func_0200c724(void *self, s32 index, s32 idx, s32 arg3);
extern void func_0200c834(void *self, s32 arg1);
extern void func_02004b00(void *self);
extern s32 func_020867ec(void *arg0, s32 arg1);
extern s32 func_0207b68c(s32 x);
extern s32 func_02007e40(void *self);
extern s32 func_02062b3c(u32 arg0);
extern void func_0200636c(void *self);
extern void func_0202c01c(void *arg0, void *arg1);

extern M2C_UNK data_020fccc8;
extern const u8 data_020ef57c[];
extern const u8 data_020ef57f[];
extern const u8 data_020ef583[];
extern const u8 data_020ef590[];
extern const u32 data_020eea28[];
extern Bytes4 data_020ee89b[];
extern const Bytes4 data_020e5c9c;
extern u8 data_020f6ba8[];
extern u8 data_020f6bab[];
extern M2C_UNK data_0210930c;
extern M2C_UNK data_02107890;
extern s32 data_027e02a8;
extern M2C_UNK data_02104d90;
extern u8 data_02104de2[];
extern u8 data_02108e6c[];
extern void *data_027e0308;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void refresh_flags(u8 *self) {
    u32 bits = func_020062b4(self) | 1;

    M2C_FIELD(self, u32 *, 0x1B8) |= bits;
    M2C_FIELD(self, s32 *, 0x194) = func_0200631c(self);
}

static inline void add_row_parts(u8 *self, u32 kind, u16 *values) {
    u8 i;
    u16 id;

    for (i = 0; i < 5; i++) {
        id = M2C_FIELD((u8 *) &data_02104d90 + kind * 0xC + i * 2, u16 *, 0x40D2);
        if (id != 0xFA) {
            func_020063d4(self, id, values);
        }
    }
    M2C_FIELD(self, u8 *, 0x6C3) = data_02108e6c[kind * 0xC];
    func_02004404(self, M2C_FIELD(self, u8 *, 0x6C3));
}

static inline u16 char_id(u8 c) {
    u16 id = 0x276;

    if (c != 0x28) {
        id = *(u16 *) (data_020f6ba8 + c * 0x24);
    }
    return id;
}

extern "C" void *func_020029c4(u8 *self, s32 a1, u32 kind, s32 a3, void *a4) {
    Bytes4 picks;
    u16 values[6];
    s32 off;
    u16 id;
    u32 i;
    u8 c;
    u16 st;

    off = kind * 0x18;
    id  = *(u16 *) (data_020ef57c + off);
    func_0200c04c(self, a1, *(u16 *) (data_020ef57c + off), a3, a4);
    M2C_FIELD(self, void **, 0) = &data_020fccc8;
    func_0204a18c(self + 0x5CE);
    func_0202be6c(self + 0x5E0);
    M2C_FIELD(self, u8 *, 0x69D) &= ~0x7F;
    M2C_FIELD(self, s32 *, 0x6A0) = 0;
    M2C_FIELD(self, s32 *, 0x6A4) = 0;
    M2C_FIELD(self, s32 *, 0x6A8) = 0;
    M2C_FIELD(self, s32 *, 0x6AC) = 0;
    M2C_FIELD(self, s32 *, 0x6B0) = 0;
    M2C_FIELD(self, u16 *, 0x6B4) = 0;
    M2C_FIELD(self, u8 *, 0x6BA)  = 0;
    M2C_FIELD(self, u8 *, 0x6BB)  = kind;
    M2C_FIELD(self, u8 *, 0x6BC)  = 0;
    M2C_FIELD(self, u8 *, 0x6BD)  = 0;
    M2C_FIELD(self, u8 *, 0x6BE)  = 0;
    M2C_FIELD(self, u8 *, 0x6BF)  = 0;
    M2C_FIELD(self, u8 *, 0x6C0)  = 0;
    M2C_FIELD(self, u8 *, 0x6C1)  = 0;
    M2C_FIELD(self, u8 *, 0x6C2)  = 0;
    M2C_FIELD(self, u8 *, 0x6C3)  = 0x28;
    M2C_FIELD(self, u8 *, 0x6C4)  = 2;
    M2C_FIELD(self, s32 *, 0x6C8) = 0;
    M2C_FIELD(self, s32 *, 0x6CC) = 0;
    M2C_FIELD(self, s32 *, 0x6E4) = 0;
    M2C_FIELD(self, u8 *, 0x6F2)  = 0;
    M2C_FIELD(self, u8 *, 0x6F3)  = 0;
    M2C_FIELD(self, u8 *, 0x6F4)  = 10;
    if (M2C_FIELD(self, u8 *, 0x6BB) == 0x55) {
        if (func_02093fa8(func_020917b4(&data_0210930c)) == 2) {
            M2C_FIELD(self, u8 *, 0x6BB) = 0x56;
        }
    }
    M2C_FIELD(self, u8 *, 0x5D9)  = data_020ef583[off];
    M2C_FIELD(self, s16 *, 0x6B8) = data_020ef57f[off];
    M2C_FIELD(self, s16 *, 0x6B6) = M2C_FIELD(self, s16 *, 0x6B8);
    for (i = 0; i < 5; i++) {
        ((u16 *) (self + 0x6E8))[i] = 0xFA;
    }
    func_02006b24(self);
    M2C_FIELD(self, s32 *, 0x194) = *(s32 *) (data_020ef590 + off);
    M2C_FIELD(self, u32 *, 0x1B8) |= data_020eea28[kind] | 1;
    M2C_FIELD(self, u32 *, 0x8) |= 0x3A0;
    func_02001990(self, 1);
    M2C_FIELD(self, s32 *, 0x84)   = 1;
    M2C_FIELD(self, void **, 0x18) = (void *) func_02004c80;
    if (M2C_FIELD(self, u16 *, 0xC) != 1) {
        c = M2C_FIELD(self, u8 *, 0x6BB);
        if (c == 0x54 || c == 0x55 || c == 0x37) {
            M2C_FIELD(self, u32 *, 0x8) |= 0x2000000;
            M2C_FIELD(self, s32 *, 0x84) = 2;
        }
    }
    c = M2C_FIELD(self, u8 *, 0x6BB);
    if (c == 0x54 || c == 0x55 || c == 0x12) {
        M2C_FIELD(self, u32 *, 0x8) |= 0x8000000;
    }
    func_020ab40c(self + 0xD8, func_0201ca1c(M2C_FIELD(self, u16 *, 0x180)));
    func_0201c778(values);
    if (kind == 0x48) {
        func_020063d4(self, M2C_FIELD(&data_02107890, u16 *, 0xA6), values);
        func_020063d4(self, M2C_FIELD(&data_02107890, u16 *, 0xA8), values);
        func_020063d4(self, M2C_FIELD(&data_02107890, u16 *, 0xAA), values);
        func_020063d4(self, M2C_FIELD(&data_02107890, u16 *, 0xAC), values);
        func_020063d4(self, M2C_FIELD(&data_02107890, u16 *, 0xAE), values);
        func_02004404(self, M2C_FIELD(&data_02107890, u16 *, 0xB0));
        refresh_flags(self);
    } else if (kind == 0x4F) {
        s32 p0 = func_0208db80(0);
        s32 p1 = func_0208db80(1);
        s32 p2 = func_0208db80(2);
        s32 p3 = func_0208db80(3);
        s32 p4 = func_0208db80(4);

        M2C_FIELD(self, u8 *, 0x6C3) = func_02062ae4(0x27);
        func_020063d4(self, p0, values);
        func_020063d4(self, p1, values);
        func_020063d4(self, p2, values);
        func_020063d4(self, p3, values);
        if (p4 != 0xFA) {
            func_020063d4(self, p4, values);
        } else {
            M2C_FIELD(self, u16 *, 0x6F0) = 0xFA;
        }
        func_02004404(self, M2C_FIELD(self, u8 *, 0x6C3));
        refresh_flags(self);
    } else if (kind >= 0x49 && kind <= 0x4E) {
        if (data_027e02a8 & 0x40) {
            if (kind >= 0x49 && kind <= 0x4B) {
                add_row_parts(self, kind, values);
            } else {
                add_row_parts(self, kind, values);
            }
        } else {
            u8 *parts = data_02104de2 + (kind - 0x49) * 6;

            if (parts[0] != 0xFA) {
                func_020063d4(self, parts[0], values);
            }
            if (parts[1] != 0xFA) {
                func_020063d4(self, parts[1], values);
            }
            if (parts[2] != 0xFA) {
                func_020063d4(self, parts[2], values);
            }
            if (parts[3] != 0xFA) {
                func_020063d4(self, parts[3], values);
            }
            if (parts[4] != 0xFA) {
                func_020063d4(self, parts[4], values);
            }
            M2C_FIELD(self, u8 *, 0x6C3) = parts[5];
            func_02004404(self, M2C_FIELD(self, u8 *, 0x6C3));
        }
        refresh_flags(self);
    } else if (kind >= 0x50 && kind <= 0x53) {
        func_02003784(self, kind, values);
        refresh_flags(self);
    } else {
        func_02006220(self, values);
    }
    if (M2C_FIELD(self, u16 *, 0x6EC) == 0x87 || M2C_FIELD(self, u16 *, 0x6EA) == 0x47) {
        func_020063d4(self, 0x87, values);
        func_020063d4(self, 0x47, values);
    }
    if (M2C_FIELD(self, u16 *, 0x6EC) == 0xA3 || M2C_FIELD(self, u16 *, 0x6EA) == 0x5E) {
        func_020063d4(self, 0xA3, values);
        func_020063d4(self, 0x5E, values);
        func_0201c858(values, 1, 0x10E7);
        func_0201c858(values, 2, 0xFFFF);
        ((CObj *) self)->f12(0x11, 0);
    }
    func_020ab574(self + 0xD8, values, 1);
    if (kind == 0x37) {
        M2C_FIELD(self, u32 *, 0x8) &= ~0x4000;
    } else {
        func_0200c85c(self, 7);
        func_0200c85c(self, 6);
    }
    if (id != 0x276) {
        s32 mode = 0;
        u8 f     = M2C_FIELD(self, u8 *, 0x6C4);

        if (((f & 2) && !(f & 1)) || (u32) (M2C_FIELD(self, u8 *, 0x6C3) - 0x26) <= 1) {
            mode = 9;
        }
        ((CObj *) self)->f9(mode, 1, 1, 0);
    }
    func_02003ac4(self);
    func_02003cb8(self);
    st = M2C_FIELD(self, u16 *, 0x180);
    if (st == 4) {
        func_0200c724(self, 0x20E, 9, 8);
        func_0200c724(self, 0x1C8, 10, 8);
    } else if (st == 0 || st == 8 || st == 0x10 || st == 0x14 || st == 0x15 || st == 1 || st == 2 || st == 3 || st == 5 ||
               st == 0xB || st == 0xC || st == 0xE || st == 0xF || st == 0x11) {
        func_0200c724(self, 0x1C8, 10, 2);
        func_0200c724(self, 0x1E2, 11, 2);
    } else if (st == 9) {
        func_0200c724(self, 0x1C8, 10, 9);
        func_0200c724(self, 0x1E2, 11, 9);
    }
    func_0200c834(self, 10);
    func_0200c834(self, 11);
    st = M2C_FIELD(self, u16 *, 0x180);
    if (st == 8 || st == 0x10 || (u32) (st - 0x14) <= 1) {
        switch (M2C_FIELD(self, u16 *, 0x6F0)) {
            case 0xE9:
                func_020063d4(self, 0xF9, 0);
                break;
            case 0xEC:
                func_020063d4(self, 0xED, 0);
                break;
            case 0xF0:
                func_020063d4(self, 0xF5, 0);
                break;
            case 0xF8:
                func_020063d4(self, 0xF6, 0);
                break;
            case 0xEB:
                func_020063d4(self, 0xF3, 0);
                break;
            case 0xF1:
                func_020063d4(self, 0xF4, 0);
                break;
        }
    }
    if (M2C_FIELD(self, u16 *, 0x180) == 0xB) {
        u16 cid = char_id(M2C_FIELD(self, u8 *, 0x6C3));

        M2C_FIELD(self, u8 *, 0x6C4) = 4;
        if (cid != 0x276) {
            func_02004b00(self);
        }
    }
    st = M2C_FIELD(self, u16 *, 0x180);
    if (st == 0 || st == 8 || st == 0x10 || st == 0x14 || st == 0x15 || st == 1 || st == 2 || st == 3 || st == 5 ||
        st == 0xB || st == 0xC || st == 0xE || st == 0xF || st == 0x11) {
        if (func_020867ec(&data_02104d90, 0x11)) {
            c = M2C_FIELD(self, u8 *, 0x6C3);
            if (c != 0x28 && data_020f6bab[c * 0x24] == 1) {
                func_02004404(self, 0x21);
            } else if (c != 0x28) {
                func_02004404(self, 7);
            }
        }
        if (func_020867ec(&data_02104d90, 9)) {
            if (func_0207b68c(M2C_FIELD(self, u8 *, 0x6BB))) {
                func_020063d4(self, 0xBC, 0);
            } else {
                func_020063d4(self, 0xCA, 0);
            }
            if (M2C_FIELD(self, u8 *, 0x6C3) != 0x28 && M2C_FIELD(self, u16 *, 0x180) != 0xB) {
                func_02004404(self, 9);
            } else if (M2C_FIELD(self, u16 *, 0x180) == 0xB) {
                func_02004404(self, 6);
            }
        }
        if (func_020867ec(&data_02104d90, 8)) {
            func_020063d4(self, 0xBB, 0);
            if (M2C_FIELD(self, u8 *, 0x6C3) != 0x28 &&
                (M2C_FIELD(self, u16 *, 0x180) == 0xB || func_02007e40(self) != 0 ||
                 (st = M2C_FIELD(self, u16 *, 0x180)) == 8 || st == 0x10 || (u32) (st - 0x14) <= 1)) {
                if (func_02062b3c(0x32)) {
                    func_02004404(self, 0x1A);
                } else {
                    func_02004404(self, 0x1C);
                }
            } else if (M2C_FIELD(self, u8 *, 0x6C3) != 0x28) {
                picks = data_020e5c9c;
                func_02004404(self, picks.b[func_02062ae4(3)]);
            }
        }
        if (func_020867ec(&data_02104d90, 7)) {
            func_020063d4(self, 0xE4, 0);
        }
    }
    if (M2C_FIELD(self, u8 *, 0x6C3) == 0x28 && M2C_FIELD(self, u8 *, 0x6BB) > 0x47 && M2C_FIELD(self, u8 *, 0x6BB) < 0x54) {
        func_02004404(self, 0x26);
    }
    if (M2C_FIELD(self, u8 *, 0x6C3) == 0x26) {
        switch (M2C_FIELD(self, u16 *, 0x6EC)) {
            case 0x76:
            case 0x79:
            case 0x7B:
            case 0xAC:
                func_02004404(self, 0x27);
                break;
        }
    }
    ((CObj *) self)->f13(1);
    func_0200636c(self);
    M2C_FIELD(self, u32 *, 0x6CC) |= data_020ee89b[M2C_FIELD(self, u8 *, 0x6BB)].b[0];
    M2C_FIELD(self, u32 *, 0x8) |= 0x20000;
    func_0202c01c(self + 0x5E0, self);
    if ((u32) (M2C_FIELD(self, u16 *, 0x180) - 0xF) <= 1) {
        M2C_FIELD(self, u16 *, 0x1A8) |= 0x1000;
    }
    M2C_FIELD(data_027e0308, void (**)(void **, void *), 0x8)(&data_027e0308, self);
    {
        Vec3 *src = (Vec3 *) (self + 0xB0);

        vec_set((Vec3 *) (self + 0x6A0), src);
    }
    return self;
}
