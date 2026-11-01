#include "m2c.h"

typedef struct Obj Obj;
typedef struct ObjVt {
    void (*f00)(Obj *);
    void (*f04)(Obj *);
    void (*f08)(Obj *);
    void (*f0c)(Obj *);
    void (*f10)(Obj *);
    void (*f14)(Obj *);
    void (*f18)(Obj *);
    void (*f1c)(Obj *);
    void (*f20)(Obj *);
    void (*f24)(Obj *);
    void (*f28)(Obj *);
    void (*f2c)(Obj *);
    void (*f30)(Obj *);
    void (*f34)(Obj *);
    void (*f38)(Obj *);
    void (*f3c)(Obj *);
    void (*f40)(Obj *);
    void (*f44)(Obj *);
    void (*f48)(Obj *);
    void (*f4c)(Obj *);
    s32 (*f50)(Obj *);
} ObjVt;
struct Obj {
    ObjVt *vt;
};

typedef struct Mgr Mgr;
typedef struct MgrVt {
    u8 *(*get)(Mgr *, s32);
} MgrVt;
struct Mgr {
    MgrVt *vt;
};

typedef struct {
    u8 pad0[2];
    u16 mask;
    u8 pad4[2];
    u8 count;
} State;

typedef struct {
    u8 count;
    u8 total;
    u8 pad[0x1a];
} Msg;

extern s32 func_020724e0(void *arg0, s32 idx);
extern void func_02072198(void *arg0);
extern s32 func_0205d0d0(void *arg0, s32 arg1, s32 arg2);
extern void func_02065600(void *arg0, s32 arg1);
extern void func_0206574c(void *arg0, s32 arg1, s32 arg2);
extern void *func_02066708(s32 arg0);
extern void func_02065be8(void *arg0, s32 arg1, s32 arg2, s32 arg3, void *arg4, s32 arg5);
extern void func_02072444(void *arg0);
extern u8 func_02072250(void *arg0);
extern void func_02072404(void *arg0, s32 arg1);
extern s32 func_020b01bc(void);
extern u16 func_020b01cc(s32 arg0);
extern void func_020affbc(void);
extern s32 func_020b006c(void);
extern void func_020536a8(void *arg0, s32 arg1);
extern void func_020b00cc(void);
extern void func_020b01a8(void);
extern void func_0207281c(void);
extern void func_0205ecc4(void *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
extern void func_0205ce5c(void *arg0, u8 arg1);
extern void func_02073054(s32 arg0);
extern s32 func_02073044(void);
extern void func_0201b318(void *arg0, u8 arg1);
extern void func_020b04f0(void *obj, u8 id, void *src, u8 size);

extern u8 data_021027f8[];
extern u8 data_02102900[];
extern Mgr *data_021092f4;
extern u8 data_02108d90[];
extern State data_02104d2c;
extern u8 data_0210794c[];
extern Obj *data_0210b348;

void func_02071b9c(u8 *self) {
    s32 old  = M2C_FIELD(self, s32 *, 0xb0);
    s32 flag = 0;
    s32 result;
    s32 count;
    s32 i;
    s32 mask;
    Msg msg;

    if (func_020724e0(self, 1) != 0) {
        if (M2C_FIELD(self, s32 *, 0xb0) != 1) {
            M2C_FIELD(self, s32 *, 0xb0) = 1;
        } else {
            func_02072198(self);
            return;
        }
    } else if (func_020724e0(self, flag) != 0) {
        if (M2C_FIELD(self, s32 *, 0xb0) == 0) {
            flag = 1;
        } else {
            M2C_FIELD(self, s32 *, 0xb0) = flag;
        }
    }

    if (func_0205d0d0(data_021027f8, 0x20, 4) != 0) {
        M2C_FIELD(self, s32 *, 0xb0) = 1;
    } else if (func_0205d0d0(data_021027f8, 0x10, 4) != 0) {
        M2C_FIELD(self, s32 *, 0xb0) = 0;
    }

    if (func_0205d0d0(data_021027f8, 2, 4) != 0 ||
        (func_0205d0d0(data_021027f8, 1, 4) != 0 && M2C_FIELD(self, s32 *, 0xb0) == 1)) {
        func_02072198(self);
        return;
    }

    if (func_0205d0d0(data_021027f8, 1, 4) != 0 && M2C_FIELD(self, s32 *, 0xb0) == 0) {
        flag = 1;
    }

    if (M2C_FIELD(self, s32 *, 0xb0) != old) {
        func_02065600(self + 0x64, 4);
        if (M2C_FIELD(self, s32 *, 0xb0) == 1) {
            func_0206574c(self + 0x64, 3, -2);
        } else {
            func_0206574c(self + 0x64, 0, -2);
        }
        func_02065be8(self + 0x64, 0x61, 0x56, 0x98, func_02066708(0x69), 1);
        if (M2C_FIELD(self, s32 *, 0xb0) == 0) {
            func_0206574c(self + 0x64, 3, -2);
        } else {
            func_0206574c(self + 0x64, 0, -2);
        }
        func_02065be8(self + 0x64, 0x101, 0xaa, 0x98, func_02066708(0x8a), 1);
        func_02065600(self + 0x64, 3);
        func_0206574c(self + 0x64, 9, -2);
    }

    func_02072444(self);
    if (M2C_FIELD(self, s32 *, 0xa4) < 3) {
        func_02072250(self);
    }

    result = 0;
    switch (M2C_FIELD(self, s32 *, 0xa4)) {
        case 0:
            break;
        case 1:
            if (func_020b01bc() == 2 && (func_020b01cc(2) != 0 || func_020b01cc(3) != 0 || func_020b01cc(4) != 0)) {
                func_02072404(self, 2);
            }
            break;
        case 2:
            if (func_020b01bc() == 2 && (func_020b01cc(2) != 0 || func_020b01cc(3) != 0 || func_020b01cc(4) != 0) &&
                flag != 0) {
                func_020affbc();
                func_02072404(self, 3);
                M2C_FIELD(self, s32 *, 0x9c) = M2C_FIELD(data_02108d90, s32 *, 0x4c8);
            }
            break;
        case 3:
            if (func_020b01bc() == 3) {
                if (func_020b01cc(4) == 0 && func_020b01cc(3) == 0 && func_020b01cc(4) == 0) {
                    func_020b01a8();
                    while (func_020b01bc() != 0 && func_020b01bc() != 5 && func_020b01bc() != 1) {
                    }
                    data_0210b348->vt->f04(data_0210b348);
                    while (data_0210b348->vt->f50(data_0210b348) == 0) {
                        data_0210b348->vt->f08(data_0210b348);
                    }
                    func_0205ecc4(data_02102900, 0x26, 1, 0, 7, 0);
                } else if (func_020b006c() != 0) {
                    result = 0x14;
                    func_020536a8(self + 0x30, 1);
                    mask  = func_020b01cc(4);
                    count = 0;
                    for (i = 0; i < func_02073044(); i++) {
                        if (mask & (1 << i)) {
                            count++;
                        }
                    }
                    M2C_FIELD(self, s32 *, 0xac) = count + 1;
                    func_020b00cc();
                    while (func_020b01bc() != 0 && func_020b01bc() != 5) {
                    }
                    data_0210b348->vt->f04(data_0210b348);
                    while (data_0210b348->vt->f50(data_0210b348) == 0) {
                        data_0210b348->vt->f08(data_0210b348);
                    }
                    func_0207281c();
                    func_0205ecc4(data_02102900, 0x21, 1, 0, 0, 1);
                } else {
                    func_020536a8(self + 0x30, 1);
                    result = (((s64) M2C_FIELD(data_02108d90, s32 *, 0x4c8) - M2C_FIELD(self, s32 *, 0x9c)) * 20) / 50 + 1;
                    if (result >= 21) {
                        result = 20;
                    }
                }
            }
            break;
        case 4: {
            s32 n;
            s32 j;
            s32 bits;
            bits               = M2C_FIELD(data_021092f4->vt->get(data_021092f4, result), u16 *, 0x66);
            data_02104d2c.mask = bits;
            n                  = result;
            for (j = n; j < func_02073044(); j++) {
                if (bits & (1 << j)) {
                    n++;
                }
            }
            data_02104d2c.count = n;
            if (n == M2C_FIELD(self, s32 *, 0xac)) {
                func_0205ce5c(data_021027f8, data_02104d2c.count);
                func_02073054(data_02104d2c.count);
                msg.total = func_02073044();
                msg.count = data_02104d2c.count;
                func_020b04f0(data_0210794c, 0x13, &msg, 0xff);
                M2C_FIELD(self, s32 *, 0xa0) = 5;
            }
            break;
        }
    }

    func_0201b318(M2C_FIELD(self, void **, 0x98), result);
}
