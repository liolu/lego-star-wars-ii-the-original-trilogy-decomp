#include "m2c.h"

typedef struct {
    s32 v;
} Fx32;

typedef struct {
    u32 pad0;
    u16 count;
} AnimInfo;
typedef struct {
    s32 first;
    u32 pad4;
    AnimInfo *info;
} Anim;

class Actor {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void play(u16 *key, u16 a2, u16 a3, u8 a4); // Slot 0x08.

    u8 pad4[0x70];
    Anim *anim;
    u8 pad78[0x34];
    u16 id;
};

typedef struct {
    Actor *actor;
    u8 pad4[0xa];
    u8 hidden;
    u8 padf[0x19];
} Slot;

class Obj {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24();
    virtual void play(u16 *key, u16 a2, u16 a3, u8 a4); // Slot 0x28.

    u8 pad4[0xd0];
    u8 sub[0xac];
    u16 mode;
    u8 pad182[0x4e];
    Slot slots[12];
    u8 pad3b0[0x313];
    u8 level;
    u8 flags;
    u8 pad6c5[0x23];
    u16 counts[8];
};

typedef struct {
    u8 pad0[3];
    u8 kind;
    u8 pad4[8];
    void *tabC;
    void *tab10;
    u8 pad14[0x10];
} LevelInfo;

typedef struct {
    u8 a;
    u8 b;
} BytePair;

typedef struct {
    u8 pad0[0x1c];
    void *tab1c;
    void *tab20;
} Cfg;

typedef struct {
    u8 data[8];
} SndEnt;

typedef struct {
    u16 v[5];
} Key5;

enum {
    FRAME_UNSET = -0x1000
}; /* frame not yet taken from an actor's animation */
enum {
    FX32_SHIFT = 12
};       /* fx32 fraction bits */
enum {
    FX32_ONE = 0x1000
};     /* 1.0 in fx32 */

extern "C" s32 func_020061bc(Obj *self, u16 *out, Cfg *cfg, u16 *key);
extern "C" u16 *func_02010c0c(void *tab, s32 type);
extern "C" u16 *func_02010c54(s32 index, s32 value);
extern "C" Cfg *func_0208dad8(void);
extern "C" void func_0209f948(void *o, Fx32 val);

extern "C" const u16 data_020e5c9c[];
extern "C" const BytePair data_020ee04c[];
extern "C" SndEnt data_020eb7b8[];
extern "C" LevelInfo data_020f6ba8[];
extern "C" u8 data_020e9664[];
extern "C" u8 data_020e9624[];
extern "C" u8 data_020e964c[];
extern "C" u8 data_020e934c[];

/*
 * Vtable slot 0x24 override (data_020fccc8 / data_020fcf28; base func_020107c8).
 * Remaps type 6/1/0 to 11/10/9 under a flag/level condition, then looks up a u16
 * key through the level tables, the config table and the per-mode tables. Returns
 * early if no key is found. For the listed modes, each visible slot actor is played
 * with a per-actor key (optionally overridden through func_020061bc). The first
 * actor's animation start frame is taken; later actors get that frame, clamped to
 * their last frame (count << 12 minus 1.0), via func_0209f948. Finally self->play
 * runs with the key, and if a frame was taken it is also applied to self->sub.
 * a2/a3/a4 are passed through unchanged to every play() call.
 */
extern "C" void func_02005c44(Obj *self, s32 type, u16 a2, u16 a3, u8 a4) {
    s32 mode;
    u16 *key = 0;
    Fx32 frame;
    frame.v = FRAME_UNSET;

    switch (type) {
        case 6:
            if (((self->flags & 2) && !(self->flags & 1)) || (u32) (self->level - 0x26) <= 1) {
                type = 11;
            }
            break;
        case 1:
            if (((self->flags & 2) && !(self->flags & 1)) || (u32) (self->level - 0x26) <= 1) {
                type = 10;
            }
            break;
        case 0:
            if (((self->flags & 2) && !(self->flags & 1)) || (u32) (self->level - 0x26) <= 1) {
                type = 9;
            }
            break;
    }

    if (data_020f6ba8[self->level].tab10 != 0) {
        key = func_02010c0c(data_020f6ba8[self->level].tab10, type);
    }
    if (key == 0 && self->counts[data_020ee04c[type].b] != 0xfa) {
        Cfg *cfg = func_0208dad8();
        if (cfg->tab1c != 0) {
            key = func_02010c0c(cfg->tab1c, type);
        }
    }
    if (key == 0 && data_020f6ba8[self->level].tabC != 0) {
        key = func_02010c0c(data_020f6ba8[self->level].tabC, type);
    }
    if (key == 0) {
        key = func_02010c0c(&data_020eb7b8[self->mode], type);
    }

    {
        mode = self->mode;
        if (mode == 8 || mode == 16 || mode == 20 || mode == 21) {
            u8 kind = data_020f6ba8[self->level].kind;
            if (kind == 2 || kind == 0) {
                key = func_02010c0c(data_020e9664, type);
                if (key == 0) {
                    key = func_02010c0c(data_020e9624, type);
                }
            } else {
                key = func_02010c0c(data_020e9624, type);
                if (key == 0) {
                    key = func_02010c0c(data_020e9664, type);
                }
            }
        } else if (mode == 11) {
            key = func_02010c0c(data_020e964c, type);
        }

        if (key == 0) {
            return;
        }

        if (mode == 0 || mode == 1 || mode == 5 || mode == 8 || mode == 11 || mode == 12 || mode == 14 || mode == 15 ||
            mode == 16 || mode == 17 || mode == 20 || mode == 21 || mode == 4) {
            Key5 out;
            Key5 tmpl;
            u32 i;
            tmpl.v[0] = data_020e5c9c[8];
            tmpl.v[1] = data_020e5c9c[9];
            tmpl.v[2] = data_020e5c9c[10];
            tmpl.v[3] = data_020e5c9c[11];
            tmpl.v[4] = data_020e5c9c[12];
            for (i = 0; i < 12; i++) {
                if (self->slots[i].actor != 0 && self->slots[i].hidden == 0) {
                    u16 *use = func_02010c54(self->slots[i].actor->id, *key);
                    if (i == 0 && self->counts[4] != 0xfa) {
                        Cfg *cfg = func_0208dad8();
                        if (cfg->tab20 != 0) {
                            s32 found;
                            out   = tmpl;
                            found = func_020061bc(self, out.v, cfg, key);
                            if (found) {
                                use = out.v;
                            }
                            if (found) {
                            } else if (data_020f6ba8[self->level].kind == 1) {
                                key = func_02010c0c(data_020e934c, type);
                                if (key == 0) {
                                    key = func_02010c0c(data_020eb7b8, type);
                                    if (func_020061bc(self, out.v, cfg, key)) {
                                        use = out.v;
                                    }
                                } else if (func_020061bc(self, out.v, cfg, key)) {
                                    use = out.v;
                                }
                            } else {
                                key = func_02010c0c(data_020eb7b8, type);
                                if (func_020061bc(self, out.v, cfg, key)) {
                                    use = out.v;
                                }
                            }
                        }
                    }
                    if (use != 0) {
                        self->slots[i].actor->play(use, a2, a3, a4);
                        if (frame.v == FRAME_UNSET) {
                            Anim *anim = self->slots[i].actor->anim;
                            frame.v    = anim != 0 ? anim->first : 0;
                        } else {
                            Anim *anim = self->slots[i].actor->anim;
                            if (frame.v >= (anim != 0 ? anim->info->count << FX32_SHIFT : 0)) {
                                frame.v = (anim != 0 ? anim->info->count << FX32_SHIFT : 0) - FX32_ONE;
                            }
                            func_0209f948(self->slots[i].actor, frame);
                        }
                    }
                }
            }
        }
    }

    self->play(key, a2, a3, a4);
    if (frame.v != FRAME_UNSET) {
        func_0209f948(self->sub, frame);
    }
}
