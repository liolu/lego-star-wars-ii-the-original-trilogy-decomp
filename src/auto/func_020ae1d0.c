#include "m2c.h"

typedef struct WmCb {
    u16 apiid;
    u16 err;
    u16 pad4;
    u16 pad6;
    u16 state;
    u16 pad_a[3];
    u16 aid;
} WmCb;

typedef struct WmState {
    u8 pad0[0xE];
    u16 bitmap;
    s32 (*cb)(WmCb *);
    u8 pad14[0x48 - 0x14];
    s32 flag;
} WmState;

void func_020ad528(const char *fmt, ...);
void func_020ad7a4(s32);
void func_020ad8b4(s32);
s32 func_020ae2f8(void);
s32 WM_Disconnect(void *callback, u16 aid);
extern char data_02100120[];
extern char data_0210014c[];
extern WmState data_0210b320;

enum {
    STATE_CHILD_CONNECTED    = 0x7,
    STATE_CHILD_DISCONNECTED = 0x9
};

/* WM_StartParent callback (registered by func_020ae170). The trace strings
 * "StartParent - child (aid %x) disconnected" and "unknown indicate, state = %d"
 * name the aid and state fields; bit is the child's bit in the connect bitmap. */
void func_020ae1d0(void *arg) {
    WmCb *cb      = (WmCb *) arg;
    const u16 bit = (u16) (1 << cb->aid);

    if (cb->err != 0) {
        func_020ad8b4(cb->err);
        func_020ad7a4(9);
        return;
    }
    switch (cb->state) {
        case STATE_CHILD_CONNECTED:
            if (data_0210b320.cb != 0) {
                if (data_0210b320.cb(cb) == 0) {
                    s32 r = WM_Disconnect(0, cb->aid);
                    if (r != 2) {
                        func_020ad8b4(r);
                        func_020ad7a4(9);
                    }
                    break;
                }
            }
            data_0210b320.bitmap |= bit;
            break;
        case STATE_CHILD_DISCONNECTED:
            func_020ad528(data_02100120, cb->aid);
            data_0210b320.bitmap &= ~bit;
            break;
        case 0:
            if (func_020ae2f8() == 0) {
                func_020ad7a4(9);
            }
            break;
        default:
            func_020ad528(data_0210014c, cb->state);
            break;
        case 2:
            break;
    }
    data_0210b320.flag = 0x29;
}
