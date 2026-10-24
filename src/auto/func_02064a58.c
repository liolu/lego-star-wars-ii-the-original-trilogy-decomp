#include "m2c.h"

typedef struct {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
    u8 b5 : 1;
    u8 b6 : 1;
    u8 b7 : 1;
} Flags;

typedef struct {
    u8 f0;
    u8 f1;
    u16 f2;
    u16 f4;
    u16 f6;
    u16 f8;
    u16 fa;
} Obj;

typedef struct {
    s16 x;
    s16 y;
} Pos;

extern Flags data_021044a4;

s32 func_02064a58(Obj *self, Pos *pos) {
    s32 valid = pos->x != 0 && pos->y != 0;
    if (valid) {
        u16 h  = self->fa;
        u16 w  = self->f8;
        u16 ry = self->f6;
        u16 rx = self->f4;
        u16 x  = pos->x;
        u16 y  = pos->y;
        if (x >= rx && x < rx + w && y >= ry && y < ry + h) {
            self->f1 = data_021044a4.b4 != 0;
            self->f2++;
            goto done;
        }
    }
    self->f1 = 0;
    self->f2 = 0;
done:
    return self->f2 != 0;
}
