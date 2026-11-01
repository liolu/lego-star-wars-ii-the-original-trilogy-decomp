/* Reconstructed from USA ARM9 at 0x0209e338 (956 bytes).
 *
 * Steps one setting of the menu row selected by the byte at self+0xac in the
 * direction dir (+1 or -1; row 3 instead reacts to dir == 0):
 *   row 0: walks the manager byte 0x40 to the next/previous of the 74 table
 *          entries whose func_02093fd0 value is ENTRY_KIND_STOP (wrapping),
 *          then sets manager word 0x34 via func_0209b6d4 for entries 4 and 5;
 *   row 1: steps manager word 0x34 in 0..4 (when byte 0x40 is 5 the values 1,
 *          3 and 4 are skipped) and stores it via func_0209b6d4;
 *   row 2: steps manager word 0x3c by 120/60/15 depending on its size and
 *          clamps it to 15..1200;
 *   row 3: sends message 0xa with func_02062ae4(5000) and the byte 3.
 * Rows 0-2 then post message MSG_STATE (word 0x3c, bytes of 0x34 and 0x40) to
 * data_0210794c and redraw via func_0209e1ac.
 */
#include "m2c.h"

typedef struct {
    u8 pad0[0x34];
    s32 f34;
    s32 pad38;
    u32 f3c;
    u8 f40;
} Mgr;

typedef struct {
    u32 f00;
    u8 f04;
    u8 f05;
    u8 pad06[0x16];
} StateMsg;

typedef struct {
    u8 pad00[4];
    u16 f04;
    u8 f06;
    u8 pad07[0x15];
} ArgMsg;

extern Mgr *data_02107860;
extern M2C_UNK data_0210794c;

u32 func_02062ae4(s32 arg0);
u8 func_02093fd0(s32 arg0);
void func_0209b6d4(void *arg0, s32 arg1);
void func_0209e1ac(void *arg0);
void func_020b04f0(void *obj, u8 id, void *src, u8 size);

enum {
    ENTRY_COUNT     = 0x4a,
    ENTRY_KIND_STOP = 0x11,
    OPTION_COUNT    = 5,
    VALUE_BIG       = 600,
    VALUE_MID       = 60,
    STEP_BIG        = 120,
    STEP_MID        = 60,
    STEP_SMALL      = 15,
    VALUE_MIN       = 15,
    VALUE_MAX       = 1200,
    MSG_STATE       = 0x1f,
};

void func_0209e338(void *self, s32 dir) {
    switch (M2C_FIELD(self, u8 *, 0xac)) {
        case 0: {
            Mgr **pp = &data_02107860;
            StateMsg msg;
            Mgr *m;
            s16 entry = (*pp)->f40;
            if (dir == 1) {
                entry++;
                while (func_02093fd0(entry) != ENTRY_KIND_STOP) {
                    entry++;
                    if (entry >= ENTRY_COUNT) {
                        entry = 0;
                    }
                }
            } else if (dir == -1) {
                entry--;
                while (func_02093fd0(entry) != ENTRY_KIND_STOP) {
                    entry--;
                    if (entry < 0) {
                        entry = ENTRY_COUNT - 1;
                    }
                }
            }
            (*pp)->f40 = entry;
            switch (entry) {
                case 4:
                    func_0209b6d4(*pp, 1);
                    break;
                case 5:
                    func_0209b6d4(*pp, 0);
                    break;
            }
            m       = *pp;
            msg.f00 = m->f3c;
            msg.f04 = m->f34;
            msg.f05 = m->f40;
            func_020b04f0(&data_0210794c, MSG_STATE, &msg, 0xff);
            func_0209e1ac(self);
            break;
        }
        case 1: {
            Mgr **pp = &data_02107860;
            StateMsg msg;
            Mgr *m    = *pp;
            s8 option = m->f34;
            s32 done;
            if (dir == 1) {
                option++;
                if (m->f40 == 5) {
                    done = 0;
                    do {
                        if (option >= OPTION_COUNT) {
                            option = 0;
                        }
                        switch (option) {
                            case 1:
                            case 3:
                            case 4:
                                option++;
                                break;
                            case 0:
                            case 2:
                                done = 1;
                                break;
                        }
                    } while (!done);
                }
            } else if (dir == -1) {
                option--;
                if (m->f40 == 5) {
                    done = 0;
                    do {
                        if (option < 0) {
                            option = OPTION_COUNT - 1;
                        }
                        switch (option) {
                            case 1:
                            case 3:
                            case 4:
                                option--;
                                break;
                            case 0:
                            case 2:
                                done = 1;
                                break;
                        }
                    } while (!done);
                }
            }
            if (option < 0) {
                option = OPTION_COUNT - 1;
            }
            if (option >= OPTION_COUNT) {
                option = 0;
            }
            func_0209b6d4(m, option);
            m       = *pp;
            msg.f00 = m->f3c;
            msg.f04 = m->f34;
            msg.f05 = m->f40;
            func_020b04f0(&data_0210794c, MSG_STATE, &msg, 0xff);
            func_0209e1ac(self);
            break;
        }
        case 2: {
            Mgr **pp = &data_02107860;
            StateMsg msg;
            Mgr *m    = *pp;
            u32 value = m->f3c;
            if (dir == -1) {
                if (value > VALUE_BIG) {
                    m->f3c = value - STEP_BIG;
                } else if (value > VALUE_MID) {
                    m->f3c = value - STEP_MID;
                } else {
                    m->f3c = value - STEP_SMALL;
                }
                if ((*pp)->f3c <= VALUE_MIN) {
                    (*pp)->f3c = VALUE_MIN;
                }
            } else if (dir == 1) {
                if (value > VALUE_BIG) {
                    m->f3c = value + STEP_BIG;
                } else if (value > VALUE_MID) {
                    m->f3c = value + STEP_MID;
                } else {
                    m->f3c = value + STEP_SMALL;
                }
                if ((*pp)->f3c >= VALUE_MAX) {
                    (*pp)->f3c = VALUE_MAX;
                }
            }
            m       = *pp;
            msg.f00 = m->f3c;
            msg.f04 = m->f34;
            msg.f05 = m->f40;
            func_020b04f0(&data_0210794c, MSG_STATE, &msg, 0xff);
            func_0209e1ac(self);
            break;
        }
        case 3: {
            ArgMsg msg;
            if (dir != 0) {
                return;
            }
            msg.f04 = func_02062ae4(0x1388);
            msg.f06 = 3;
            func_020b04f0(&data_0210794c, 0xa, &msg, 0xff);
            break;
        }
    }
}
