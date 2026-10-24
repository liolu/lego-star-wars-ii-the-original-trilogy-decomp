#include "m2c.h"

typedef struct {
    u16 low : 7;
    u16 skip : 1;
    u16 row : 5;
    u16 pad : 1;
    u16 size : 2;
} TileInfo;

extern u32 data_027e02a8;
extern void func_02053010(TileInfo, u8, s32, s32, s32);
extern void MI_DmaFill16(u32, void *, u16, u32);

void func_020530b0(s32 a, s32 b, s32 c, TileInfo info, u8 mode) {
    s32 bank;
    u32 len;

    if (data_027e02a8 & 0x200) {
        bank = 0;
    } else {
        bank = c;
    }
    func_02053010(info, mode, a, b, c);
    if (!info.skip) {
        a |= (b >> 4) << 12;
    }
    if (mode) {
        switch (info.size) {
            case 0:
                len = 0x200;
                break;
            case 1:
                len = 0x800;
                break;
            case 2:
                len = 0x2000;
                break;
            case 3:
                len = 0x8000;
                break;
        }
    } else {
        switch (info.size) {
            case 0:
                len = 0x800;
                break;
            case 1:
            case 2:
                len = 0x1000;
                break;
            case 3:
                len = 0x2000;
                break;
        }
    }
    MI_DmaFill16(3, (void *) (0x6000000 + bank * 0x200000 + info.row * 0x800), (u16) a, len);
}
