#include "m2c.h"

typedef struct {
    u8 pad0[0x6bb];
    u8 f_6bb;
} Ent;

typedef struct {
    u8 pad0[0x4c];
    Ent *slots[4];
} Globals;

typedef struct {
    u8 pad0[4];
    u16 vals[4];
} SlotRow;

typedef struct {
    u8 pad0[4];
    s32 id;
} Inner;

typedef struct {
    u8 pad0[4];
    Inner *inner;
    u8 pad8[0x68 - 8];
    s32 mode;
} Game;

typedef struct {
    u8 pad0[8];
    s32 type;
    s32 f_0c;
    s32 f_10;
    s32 f_14;
    u8 pad18[0x2c - 0x18];
    u8 sub[0xbc - 0x2c];
    u8 f_bc;
} Self;

typedef struct {
    u8 pad0[0xc];
} Item;

typedef struct VObj VObj;
typedef struct {
    u8 pad0[0x18];
    void (*fn18)(VObj *, s32);
} VTable;
struct VObj {
    VTable *vt;
};

extern void func_02053f44(void *, s32);
extern void func_02060158(void *, s32, s32, s32, ...);
extern void func_02063b94(void *);
extern void func_02068148(void *);
extern void func_02068268(void *, u16);
extern void func_0206adbc(void *);
extern void func_0207f4c4(void *);
extern void func_0208562c(void *);
extern s32 func_02085948(void *);
extern void func_02085e74(void *, s32, s32, s32);
extern void func_020861bc(void *, s32, s32);
extern s32 func_020867ec(void *, s32);
extern void func_020897f8(void *);
extern void func_02089a6c(void *);
extern void func_02089a78(void *);
extern void func_02089ba8(void *);
extern void func_02089c88(void *);
extern void func_02089e60(void *);
extern void func_0208aaf0(void *);
extern void func_0208bf74(void *);
extern void func_0208c628(void *);
extern void func_0208cfb0(s32);
extern void func_0208cfdc(void);
extern void func_0208d478(void *, s32);
extern void func_02090ae8(void *, s32, s32);
extern void func_02091310(void *, s32);
extern u8 func_020917b4(void *);
extern void *func_020917c8(Globals *, s32, s32);
extern void func_02093690(Globals *, Ent *);
extern u16 func_02093f80(s32);
extern u8 func_02093f94(s32);
extern u8 func_02093fa8(s32);
extern u8 func_02093fd0(s32);
extern void func_020a99bc(void *, u8);
extern u8 *func_020aaeb4(void);
extern void func_020ac278(void *, s32, s32);
extern void func_020aca00(void *, s32);
extern void func_020b3ab0(void *);
extern void func_020b9c4c(void *, s32);
extern void func_020b9c90(void *);

extern s32 data_02101b70;
extern M2C_UNK data_02101b78;
extern M2C_UNK data_021027f8;
extern Game data_02102900;
extern M2C_UNK data_02104364;
extern SlotRow data_02104388[];
extern M2C_UNK data_02104434;
extern u32 data_02104d90;
extern Item data_02104d94[];
extern M2C_UNK data_02104dc4;
extern M2C_UNK data_02104df4;
extern M2C_UNK data_02106d90;
extern VObj *data_021092f4;
extern Globals data_0210930c;
extern M2C_UNK data_021093a0;
extern M2C_UNK data_0210db30;
extern u32 data_027e02a8;
extern M2C_UNK data_027e0864;
extern M2C_UNK data_027e0874;
extern M2C_UNK data_027e0bd0;

static inline s32 game_inner_id(void) {
    Inner *in = data_02102900.inner;
    return in != NULL ? in->id : 0;
}

/* Reached only through the pointer table data_020ff478.  Runs a long setup
 * sequence over several global subsystems, branching on self->type: resets the
 * four 12-byte entries at data_02104d94 when func_02093fd0(self->f_0c) is 0x10,
 * starts func_020ac278 on data_027e0874 unless data_02102900 reports 0x1e,
 * calls per-type helpers on data_02104df4 (types 4, 0xe/0x12, 0xf, 0x10),
 * walks the slot table of data_0210930c (two slots for type 6, four always),
 * then picks func_020aca00 / func_0208d478 arguments from func_020867ec flag
 * queries on data_02104d90 before the final func_02060158 call. */
void func_0208c888(Self *self) {
    u32 i;

    func_0207f4c4(M2C_FIELD(&data_02106d90, void **, 0xad8));

    if (func_02093fd0(self->f_0c) == 0x10) {
        for (i = 0; i < 4; i++) {
            func_0208562c(&data_02104d94[i]);
        }
        func_0208c628(&data_02104dc4);
        func_020861bc(&data_02104d90, 10, 0);
    }
    if (self->type == 5) {
        func_02085948(&data_02104d90);
    }
    func_0208cfdc();

    if (data_02102900.mode != 0x1e && game_inner_id() != 0x1e) {
        if (func_02093fd0(self->f_0c) == 0x10) {
            func_020ac278(&data_027e0874, 6, 0xfa00);
        } else {
            func_020ac278(&data_027e0874, 1, 0x1d4c0);
        }
    }

    func_02068148(&data_027e0864);
    func_020b3ab0(&data_027e0bd0);
    func_02068268(&data_027e0864, func_02093f80(self->f_0c));
    func_02053f44(&data_02101b78, 0x100);
    func_02085e74(&data_02104d90, self->f_0c, self->f_14, self->f_10);
    data_02101b70 = 0x1000;

    if (self->type == 0xe || self->type == 0x12) {
        func_02089a6c(&data_02104df4);
    } else if (self->type == 4) {
        func_02089a78(&data_02104df4);
    }
    func_02090ae8(&data_0210930c, self->f_0c, self->f_14);
    if (self->type == 0x10) {
        func_02089ba8(&data_02104df4);
    } else {
        func_02091310(&data_0210930c, self->f_10);
        data_02104d90 &= ~1;
    }
    if (func_02093fa8(func_020917b4(&data_0210930c)) == 1) {
        func_020b9c4c(&data_0210db30, 1);
    } else {
        func_020b9c4c(&data_0210db30, 0);
    }
    func_02063b94(&data_02104434);
    func_020b9c90(&data_0210db30);

    if ((data_027e02a8 & 0x40) != 0 && (u32) (self->type - 0xe) <= 1) {
        data_021092f4->vt->fn18(data_021092f4, 1);
    }

    /* Type 6: refresh slots 0 and 1 whose row value differs from the slot's
     * own byte at 0x6bb, skipping the value 100. */
    if (self->type == 6) {
        SlotRow *row = &data_02104388[M2C_FIELD(&data_02104364, u8 *, 0x4e)];
        for (i = 0; i < 2; i++) {
            Ent *e = data_0210930c.slots[i];
            if (e != NULL) {
                u16 v = row->vals[i];
                if (v != 100 && v != e->f_6bb) {
                    func_020917c8(&data_0210930c, i, v);
                }
            }
        }
    }

    if (self->type == 0x10) {
        func_0208cfb0(M2C_FIELD(&data_021027f8, u8 *, 0x2a));
        func_020a99bc(func_020aaeb4(), 6);
        func_020aaeb4()[0x25c] = 1;
    } else {
        func_0208cfb0(M2C_FIELD(&data_021027f8, u8 *, 0x2a));
    }

    for (i = 0; i < 4; i++) {
        Ent *e = data_0210930c.slots[i];
        if (e != NULL && self->f_bc == 0) {
            func_02093690(&data_0210930c, e);
        }
    }

    if (self->type == 0xe || self->type == 0x12) {
        func_02089c88(&data_02104df4);
    } else if (self->type == 0xf) {
        func_020897f8(&data_02104df4);
    } else if (self->type == 4) {
        func_02089e60(&data_02104df4);
    }

    if (self->type != 0x10 && data_02102900.mode != 0x1e && game_inner_id() != 0x1e && func_020867ec(&data_02104d90, 4) == 0) {
        func_0208aaf0(self->sub);
    } else {
        func_0208bf74(self->sub);
    }

    M2C_FIELD(&data_02106d90, u8 *, 0xba4) = func_02093f94(func_020917b4(&data_0210930c));

    if (func_020867ec(&data_02104d90, 4) != 0) {
        func_0206adbc(M2C_FIELD(&data_02106d90, void **, 0xad4));
    }

    if (func_020867ec(&data_02104d90, 6) != 0) {
        func_020aca00(&data_027e0874, 0xb);
    } else if (func_020867ec(&data_02104d90, 2) != 0 || (data_02104d90 & 2) != 0) {
        func_020aca00(&data_027e0874, 0xc);
    } else if (func_020867ec(&data_02104d90, 5) != 0) {
        func_020aca00(&data_027e0874, 9);
    }

    if (game_inner_id() != 0x1e && data_02102900.mode != 0x1e) {
        if (func_020867ec(&data_02104d90, 7) != 0) {
            func_0208d478(&data_021093a0, 1);
        } else if (func_020867ec(&data_02104d90, 8) != 0) {
            func_0208d478(&data_021093a0, 2);
        } else if (func_020867ec(&data_02104d90, 5) != 0) {
            func_0208d478(&data_021093a0, 0);
        }
    }

    func_02060158(&data_02102900, 1, 0x13, 1, self->f_0c);
}
