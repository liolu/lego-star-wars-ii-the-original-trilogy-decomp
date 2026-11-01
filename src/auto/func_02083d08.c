#include "m2c.h"

typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    HalfWord a;
    HalfWord b;
} HwPair;

/* Sub-object at +0x30 (vtable data_020fe710, destroyed by func_02062748). */
typedef struct {
    void *vtable;
    s32 unk4;
    s32 unk8;
    s32 unkC;
    s32 unk10;
    s32 unk14;
    s32 unk18;
    s32 unk1C;
    s32 unk20;
    u8 lines[0xc00];
    u16 scale;
    u16 unkC26;
    u16 unkC28;
    u8 padC2A[2];
    void *buffer;
    void *hwReg;
    u8 enabled;
    u8 padC35[3];
} SubObj;

extern void *func_02062694(void *, M2C_UNK, s8);
extern void func_020534e0(void *);
extern void func_0206d4d0(void *, M2C_UNK, M2C_UNK, M2C_UNK, s32);
extern void func_0206549c(s32 *);
extern M2C_UNK data_020fe6d0;
extern M2C_UNK data_020ff3b0;
extern M2C_UNK data_020ff3d4;
extern M2C_UNK data_020fe710;

static inline void hw_pair_set(HwPair *p, u16 v) {
    *(u16 *) &p->a = v;
    *(u16 *) &p->b = v;
}

/* Constructor: base vtables, the scanline sub-object at +0x30, the member objects at +0xc68..+0xcd8, then defaults. */
void *func_02083d08(void *self, s32 a1, s32 a2, s32 a3, s32 a4) {
    M2C_FIELD(self, M2C_UNK **, 0x0) = &data_020fe6d0;
    M2C_FIELD(self, s32 *, 0x4)      = a1;
    M2C_FIELD(self, s32 *, 0x8)      = a2;
    M2C_FIELD(self, s32 *, 0xc)      = a3;
    M2C_FIELD(self, s32 *, 0x10)     = a4;
    M2C_FIELD(self, s32 *, 0x14)     = 0;
    M2C_FIELD(self, s32 *, 0x18)     = 0;
    M2C_FIELD(self, s32 *, 0x1c)     = 0;
    M2C_FIELD(self, s32 *, 0x20)     = 0;
    M2C_FIELD(self, s32 *, 0x24)     = 0;
    func_02062694((u8 *) self + 0x28, 1, 2);
    M2C_FIELD(self, M2C_UNK **, 0x0)  = &data_020ff3b0;
    M2C_FIELD(self, M2C_UNK **, 0x28) = &data_020ff3d4;
    {
        SubObj *s = (SubObj *) ((u8 *) self + 0x30);
        HwPair scale;

        func_02062694(s, 1, 2);
        s->vtable  = &data_020fe710;
        s->unk8    = 0;
        s->unkC    = 0;
        s->unk10   = 0;
        s->unk14   = 0;
        s->unk18   = 0x60000;
        s->unk1C   = 0x60000;
        s->unk20   = 0x60000;
        s->enabled = 0;
        s->scale   = 0x2000;
        hw_pair_set(&scale, 0x2000); // FAKE: dead local pair; keeps the two unread stack halfword stores (sp+4, sp+6)
    }
    func_020534e0((u8 *) self + 0xc68);
    func_020534e0((u8 *) self + 0xc70);
    func_020534e0((u8 *) self + 0xc78);
    func_0206d4d0((u8 *) self + 0xc80, 0, 1, 2, 3);
    func_0206d4d0((u8 *) self + 0xcac, 1, 4, 0, 1);
    func_0206549c((s32 *) ((u8 *) self + 0xcd8));
    M2C_FIELD(self, s32 *, 0xd08) = 0;
    M2C_FIELD(self, u16 *, 0xd0c) = 0;
    M2C_FIELD(self, u16 *, 0xd0e) = 0x2e36;
    M2C_FIELD(self, s32 *, 0xd10) = 0x4000;
    M2C_FIELD(self, s32 *, 0xd14) = 0;
    M2C_FIELD(self, s32 *, 0xd18) = 0x5d000;
    M2C_FIELD(self, s32 *, 0xd1c) = 0;
    M2C_FIELD(self, s32 *, 0xd20) = -1;
    M2C_FIELD(self, s32 *, 0xd24) = -1;
    M2C_FIELD(self, s32 *, 0xd28) = 0;
    M2C_FIELD(self, s32 *, 0xd2c) = 0;
    M2C_FIELD(self, s32 *, 0xd30) = 0;
    M2C_FIELD(self, s32 *, 0xd34) = M2C_FIELD(self, s32 *, 0xc);
    M2C_FIELD(self, u8 *, 0xeb8)  = 0;
    M2C_FIELD(self, u8 *, 0xeb9)  = 0;
    M2C_FIELD(self, u8 *, 0xeba)  = 0;
    return self;
}
