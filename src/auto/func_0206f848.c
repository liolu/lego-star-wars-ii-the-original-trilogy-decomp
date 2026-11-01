#include "m2c.h"

typedef struct {
    u8 language;
    u8 favoriteColour;
    u8 birthday[2];
    u16 nickName[10];
    u16 nickNameLength;
    u16 comment[26];
    u16 commentLength;
} OSOwnerInfo;

typedef struct {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
    u8 b5 : 1;
    u8 b6 : 1;
    u8 b7 : 1;
    u8 pad[0x29];
    s16 x;
    s16 y;
} Touch;

typedef struct {
    u8 pad0[6];
    u8 count;
} Info;

typedef struct {
    u8 pad00[0x5c];
    u8 f5c[0x38];
    s32 f94;
    u8 pad98[4];
    s32 f9c;
    s32 fa0;
} Obj;

typedef struct {
    u8 a;
    u8 b;
    u8 rest[0x1a];
} Msg;

typedef struct VObj {
    void (**vt)(struct VObj *, s32);
} VObj;

extern void OS_GetOwnerInfo(OSOwnerInfo *info);
extern void func_0206fbf4(Obj *);
extern void func_0205ce5c(void *arg0, s32 arg1);
extern void func_02073054(s32 arg0);
extern s32 func_02073044(void);
extern void func_020b04f0(void *obj, u8 id, void *src, u8 size);
extern s32 func_0205d0d0(void *arg0, s32 arg1, s32 arg2);
extern void func_0206fb40(Obj *arg0);
extern void func_0206fbb4(Obj *arg0);
extern void func_02065600(void *arg0, s32 arg1);
extern void func_0206574c(void *arg0, s32 arg1, s32 arg2);
extern s32 func_02065be8(void *arg0, s32 arg1, s32 arg2, s32 arg3, void *arg4, s32 arg5);
extern void *func_02066708(s32 arg0);

extern Info data_02104d2c;
extern u8 data_021027f8;
extern u8 data_0210794c;
extern VObj *data_021092f4;
extern Touch data_021044a4;

enum {
    KEY_A     = 0x1,         /* NDS pad masks passed to func_0205d0d0 */
    KEY_RIGHT = 0x10,
    KEY_LEFT  = 0x20,
    BOX_L_X0  = 0x46,     /* touch box that selects choice 1 */
    BOX_L_X1  = 0x78,
    BOX_R_X0  = 0x9a,     /* touch box that selects choice 0 */
    BOX_R_X1  = 0xcc,
    BOX_Y0    = 0x90,
    BOX_Y1    = 0xae,
};

/*
 * Per-frame update of a two-choice prompt. self->f9c holds the current
 * choice (1 = left, 0 = right). If the count byte in data_02104d2c changed
 * since the last frame (and is > 1), a message 0x13 carrying it is sent.
 * Touch (data_021044a4 bit 4, x/y at 0x2a/0x2c) inside a box moves the
 * choice; touching the box of the choice already held confirms it
 * (func_0206fb40 for 1, func_0206fbb4 for 0). LEFT/RIGHT keys move the
 * choice and A confirms it. When the choice changed, the two labels
 * (text 0x69 at x 0x56, text 0x8a at x 0xaa, y 0x98) are redrawn into
 * self->f5c with the selected one in style 3, the other in style 0.
 */
void func_0206f848(Obj *self) {
    OSOwnerInfo info;
    Msg msg;
    s32 prev;
    s32 pos;

    prev = self->f9c;
    OS_GetOwnerInfo(&info);
    func_0206fbf4(self);
    if (data_02104d2c.count > 1 && self->f94 != data_02104d2c.count) {
        func_0205ce5c(&data_021027f8, data_02104d2c.count);
        func_02073054(data_02104d2c.count);
        msg.b = func_02073044();
        msg.a = data_02104d2c.count;
        func_020b04f0(&data_0210794c, 0x13, &msg, 0xff);
    }
    data_021092f4->vt[0](data_021092f4, 0);
    if (data_021044a4.b4) {
        s16 x = data_021044a4.x;
        s16 y = data_021044a4.y;
        if (x >= BOX_L_X0 && x < BOX_L_X1 && y >= BOX_Y0 && y < BOX_Y1 && self->f9c == 1) {
            func_0206fb40(self);
        } else if (x >= BOX_L_X0 && x < BOX_L_X1 && y >= BOX_Y0 && y < BOX_Y1) {
            self->f9c = 1;
        } else if (x >= BOX_R_X0 && x < BOX_R_X1 && y >= BOX_Y0 && y < BOX_Y1 && self->f9c == 0) {
            func_0206fbb4(self);
        } else if (x >= BOX_R_X0 && x < BOX_R_X1 && y >= BOX_Y0 && y < BOX_Y1) {
            self->f9c = 0;
        }
    }
    if (func_0205d0d0(&data_021027f8, KEY_LEFT, 4)) {
        self->f9c = 1;
    } else if (func_0205d0d0(&data_021027f8, KEY_RIGHT, 4)) {
        self->f9c = 0;
    }
    if (func_0205d0d0(&data_021027f8, KEY_A, 4)) {
        if (self->f9c == 1) {
            func_0206fb40(self);
        } else if (self->f9c == 0) {
            func_0206fbb4(self);
        }
    }
    if (self->f9c != prev) {
        func_02065600(self->f5c, 4);
        if (self->f9c == 1) {
            func_0206574c(self->f5c, 3, -2);
        } else {
            func_0206574c(self->f5c, 0, -2);
        }
        pos = self->fa0;
        pos = func_02065be8(self->f5c, pos, 0x56, 0x98, func_02066708(0x69), 1);
        if (self->f9c == 0) {
            func_0206574c(self->f5c, 3, -2);
        } else {
            func_0206574c(self->f5c, 0, -2);
        }
        func_02065be8(self->f5c, pos, 0xaa, 0x98, func_02066708(0x8a), 1);
        func_02065600(self->f5c, 3);
        func_0206574c(self->f5c, 9, -2);
    }
    self->f94 = data_02104d2c.count;
}
