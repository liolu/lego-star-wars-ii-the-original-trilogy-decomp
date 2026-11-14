#include "m2c.h"

typedef s32 BOOL;

typedef struct {
    u8 pad0[0xA];
    u8 flags;
} UnkDb14;

typedef struct {
    u8 pad0[0x40];
    u8 fogEnable;
    u8 pad41[3];
    u16 fogSlope;
    u16 fogOffset;
} UnkFog;

extern M2C_UNK data_02102900;
extern M2C_UNK data_0210930c;
extern UnkDb14 data_0210db14;
extern s32 data_027e02a8;
extern UnkFog data_027e0874;

extern void func_0205ecc4(M2C_UNK *arg0, M2C_UNK arg1, M2C_UNK arg2, M2C_UNK arg3, s32 arg4, s32 arg5);
extern void func_0206adbc(void *arg0);
extern void func_0206aecc(void *arg0);
extern u8 func_0206af20(void *arg0);
extern s32 func_020867ec(s32 arg0, s32 arg1);
extern M2C_UNK func_02088b84(M2C_UNK *arg0, u8 arg1);
extern M2C_UNK func_020917b4(M2C_UNK *arg0);
extern M2C_UNK func_02093c9c(M2C_UNK *arg0, M2C_UNK arg1);
extern void func_020aca00(void *arg0, s32 arg1);
extern void func_020b90e4(u32 arg0, u8 arg1);
extern void G3X_SetFog(BOOL enable, s32 fogMode, s32 fogSlope, s32 fogOffset);

/* 32-bit words holding one bit per flag id, at self + 0x4518. */
enum {
    FLAG_WORDS = 0x4518
};

static inline int FlagWord(int id) {
    return id >> 5;
}

static inline u8 Db14Flags(const UnkDb14 *p) {
    return p->flags;
}

static inline void SetFlag(u8 *self, s32 id, s32 value) {
    u32 *flags          = (u32 *) (self + FLAG_WORDS);
    flags[FlagWord(id)] = (flags[FlagWord(id)] & ~(1 << (id & 0x1F))) | (value << (id & 0x1F));
}

static inline s32 IsFlagSet(u8 *self, s32 index, s32 shift) {
    s32 set = 1;
    if (!(M2C_FIELD(self + index * 4, s32 *, FLAG_WORDS) & (1 << shift))) {
        set = 0;
    }
    return set;
}

static inline void StoreFlag(u8 *self, s32 id, s32 shift, s32 value) {
    u32 *flags = (u32 *) (self + FLAG_WORDS);
    flags[FlagWord(id)] &= ~(1 << shift);
    flags[FlagWord(id)] |= value << shift;
}

static inline void Refresh(u8 *self, s32 notify) {
    if (notify != 0) {
        if (data_027e02a8 & 3) {
            func_02093c9c(&data_0210930c, 0);
        }
        func_02088b84((M2C_UNK *) (self + 0x64), func_020917b4(&data_0210930c));
        func_0205ecc4(&data_02102900, 4, 0, 0x12, self[0x28D0], 0);
        func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
    }
}

static inline void RefreshAll(u8 *self, s32 notify) {
    if (notify != 0) {
        u8 i;
        if (data_027e02a8 & 3) {
            func_02093c9c(&data_0210930c, 0);
        }
        func_02088b84((M2C_UNK *) (self + 0x64), func_020917b4(&data_0210930c));
        for (i = 0; i < 4; i++) {
            *(u16 *) (self + i * 0x40 + 0x2250) = 0xFA;
        }
        func_0205ecc4(&data_02102900, 4, 0, 0x12, self[0x28D0], 0);
        func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
    }
}

static inline void FogOff(void) {
    data_027e0874.fogEnable = 0;
    G3X_SetFog(0, 0, data_027e0874.fogSlope, data_027e0874.fogOffset);
}

/*
 * Toggles bit `id` in the flag words and returns its new state. Setting any of
 * ids 2, 5, 7, 8, 9 or 14-17 first clears the others in that group (and 6).
 */
s32 func_02086810(u8 *self, s32 id, s32 notify) {
    s32 shift = id & 0x1F;
    s32 index = id >> 5;
    s32 state;

    if (IsFlagSet(self, index, shift) ^ 1) {
        state = 1;
    } else {
        state = 0;
    }
    {
        u32 *flags = (u32 *) (self + FLAG_WORDS);
        flags[index] &= ~(1 << shift);
        // FAKE: same word as flags[index] above, respelled as id >> 5; forces the target's register colouring
        flags[id >> 5] |= state << shift;
    }

    if (state != 0) {
        switch (id) {
            case 2:
            case 5:
            case 7:
            case 8:
            case 9:
            case 14:
            case 15:
            case 16:
            case 17:
                if (Db14Flags(&data_0210db14) & 0x20) {
                    data_0210db14.flags &= ~0x20;
                }
                switch (id) {
                    case 2:
                    case 7:
                    case 8:
                    case 9:
                        switch (id) {
                            case 9:
                                func_020b90e4(0x15, 0xFF);
                                break;
                            case 8:
                                func_020b90e4(0x1B, 0xFF);
                                break;
                            case 7:
                                func_020b90e4(0x1A, 0xFF);
                                break;
                            case 2:
                                func_020b90e4(0x1C, 0xFF);
                                break;
                        }
                        data_0210db14.flags |= 0x20;
                        break;
                }
                SetFlag(self, 9, 0);
                SetFlag(self, 8, 0);
                SetFlag(self, 7, 0);
                SetFlag(self, 14, 0);
                SetFlag(self, 15, 0);
                SetFlag(self, 16, 0);
                SetFlag(self, 17, 0);
                SetFlag(self, 6, 0);
                SetFlag(self, 5, 0);
                SetFlag(self, 2, 0);
                StoreFlag(self, id, shift, state);
                if (id == 2) {
                    func_020aca00(&data_027e0874, 0xC);
                }
            case 13:
                RefreshAll(self, notify);
                break;
            case 4:
                if (func_0206af20(*(void **) (self + 0x2AD4)) == 0) {
                    func_0206adbc(*(void **) (self + 0x2AD4));
                }
                break;
            case 3:
            case 12:
                Refresh(self, notify);
                break;
            case 6:
                if (func_020867ec((s32) self, 2) != 0) {
                    SetFlag(self, 2, 0);
                    RefreshAll(self, notify);
                }
                func_020aca00(&data_027e0874, 0xB);
                break;
        }
    } else {
        switch (id) {
            case 4:
                if (func_0206af20(*(void **) (self + 0x2AD4)) != 0) {
                    func_0206aecc(*(void **) (self + 0x2AD4));
                }
                break;
            case 2:
                FogOff();
            case 7:
            case 8:
            case 9:
                data_0210db14.flags &= ~0x20;
            case 5:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
                RefreshAll(self, notify);
                break;
            case 3:
            case 12:
                Refresh(self, notify);
                break;
            case 6:
                FogOff();
                break;
        }
    }
    return state;
}
