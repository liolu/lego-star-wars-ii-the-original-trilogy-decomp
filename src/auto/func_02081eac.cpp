#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

class Part {
public:
    virtual void unknown00();
    virtual void release(); // Slot 0x04.
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24(s32, s32, s32, s32); // Slot 0x24.

    u8 pad04[0x8 - 0x4];
    s32 flags;
    u8 pad0c[0xBC - 0xC];
    Vec3 scale;
    u8 padc8[0xD1 - 0xC8];
    u8 dirty;
    u8 padd2[0xD8 - 0xD2];
    u8 sub[4];
};

typedef struct Owner {
    u8 pad0[0x28];
    Part *front[8];
    u8 pad1[0xC0 - 0x48];
    Part *back[8];
    u8 pad2[0xEC - 0xE0];
    s32 cur[7];
    s32 sel;
    s32 tab[6];
    s32 f124;
    s32 selected;
    u8 pad3[0x134 - 0x12C];
    u8 *lists[5];
    s32 counts[6];
    s32 count5;
} Owner;

typedef struct Cfg {
    u8 pad[0x24];
    Vec3 pos;
    Vec3 scale;
    s16 r0;
    s16 r1;
    s16 r2;
    u8 gap[2];
    u8 f44;
    u8 f45;
} Cfg;

typedef struct Info {
    u8 pad[4];
    u16 f4;
    u16 f6;
} Info;

extern "C" void *func_0205e80c(s32, s32, s32);
extern "C" Part *func_0200ff50(void *, s32, s32, s32, Cfg *);
extern "C" u8 func_02083340(void *, s32);
extern "C" s32 func_020833b0(void *, s32);
extern "C" s32 func_0208da0c(u16, void *);
extern "C" Info *func_0208dad8(s32);
extern "C" s32 func_0208d854(s32, void *);
extern "C" s32 func_0208da5c(u16, void *);
extern "C" u16 func_0201ca1c(u16);
extern "C" s32 func_020ab40c(void *, u16);
extern "C" void func_0201c778(u16 *);
extern "C" void func_0201c858(u16 *, s32, u16);
extern "C" s32 func_020ab574(void *, u16 *, s32);

extern u8 data_020f020c[];
extern u8 data_021027f8[];
extern void *data_0210930c[];
extern s32 data_020ff2d4[];

enum {
    FX32_ONE         = 0x1000, // 1.0 in 20.12 fixed point (part config scale).
    FRONT_PART_MODEL = 0x20F,  // Model id of the overlay part put in front[slot].
};

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void part_set_scale(Part *p, const Vec3 *v) {
    if (vec_ne(&p->scale, v)) {
        vec_set(&p->scale, v);
        p->dirty = 1;
    }
}

static inline void part_set_scale_all(Part *p, s32 s) {
    Vec3 v;
    Vec3 *pv = &v;
    pv->x    = s;
    pv->y    = s;
    pv->z    = s;
    part_set_scale(p, pv);
}

static inline Part *part_new(s32 id) {
    Part *p = (Part *) func_0205e80c(0x1BC, 3, 4);
    if (p != 0) {
        Cfg cfg;
        cfg.pos.x   = 0;
        cfg.pos.y   = 0;
        cfg.pos.z   = 0;
        cfg.scale.x = FX32_ONE;
        cfg.scale.y = FX32_ONE;
        cfg.scale.z = FX32_ONE;
        cfg.r0      = 0;
        cfg.r1      = 0;
        cfg.r2      = 0;
        cfg.f44     = 1;
        cfg.f45     = 1;
        p           = func_0200ff50(p, 4, id, 1, &cfg);
    }
    return p;
}

static inline void front_release(Owner *o, s32 slot) {
    Part *f = o->front[slot];
    if (f != 0) {
        f->flags |= 2;
        o->front[slot] = 0;
    }
}

static inline void *cur_player(void) {
    return data_0210930c[data_021027f8[0x2A] + 0x13];
}

// Rebuilds the two display slots of list row `row` (slots (row % 4) * 2 and +1):
// releases the old back part (virtual slot 0x04) and flags/clears the front part,
// then, for each in-range entry that is not the current one, creates a new back part
// for the entry's model and sets its uniform scale (0x5000 on page 5, else the
// per-page data_020ff2d4 value). When either player check (func_0208d854 /
// func_0208da5c, func_0208da0c on page 5) fails, a FRONT_PART_MODEL part is
// created in front[slot]. `tint` (cleared for some page-0 ids) gates the
// func_0201ca1c / func_0201c858 colour setup on the new back part.
extern "C" void func_02081eac(Owner *self, s32 row) {
    s32 id;
    u16 model;
    s32 idx;
    s32 count;
    s32 n;
    s32 i;
    s32 k;
    Info *info;
    u8 tint;
    s32 start;
    s32 slot;

    if (self->sel == 5) {
        count = self->count5;
    } else {
        count = self->counts[data_020f020c[self->sel]];
    }
    idx   = row * 2;
    start = idx;
    n     = 2;
    if (idx + 2 > count) {
        n = count - idx;
    }
    slot = (row % 4) * 2;
    for (i = 0; i < 2; slot++, idx++, i++) {
        Part *p;

        p = self->back[slot];
        if (p != 0) {
            p->release();
        }
        self->back[slot] = 0;
        front_release(self, slot);
        if (i >= n) {
            continue;
        }
        if (self->sel == 5) {
            s32 id5;
            s32 model5;

            if (self->cur[5] == idx) {
                continue;
            }
            if (idx >= self->count5) {
                continue;
            }
            id5              = func_02083340(self, idx);
            model5           = func_020833b0(self, idx);
            self->back[slot] = part_new(model5);
            self->back[slot]->unknown24(0x61, 1, 1, 0);
            part_set_scale_all(self->back[slot], 0x5000);
            if (func_0208da0c(id5, cur_player()) != 0) {
                continue;
            }
            front_release(self, slot);
            p                 = part_new(FRONT_PART_MODEL);
            self->front[slot] = p;
            part_set_scale_all(p, 0x5000);
        } else {

            k = data_020f020c[self->sel];
            if (idx == self->cur[k]) {
                continue;
            }
            if (idx >= self->counts[k]) {
                continue;
            }
            id    = self->lists[k][start + i];
            info  = func_0208dad8(id);
            model = info->f4;
            tint  = 1;
            if (func_0208d854(id, cur_player()) == 0 || func_0208da5c(id, cur_player()) == 0) {
                front_release(self, slot);
                p                 = part_new(FRONT_PART_MODEL);
                self->front[slot] = p;
                part_set_scale_all(p, data_020ff2d4[k]);
            }
            switch (k) {
                case 0:
                    if (id == 5) {
                        model = 0x258;
                    } else if (id == 0xC) {
                        model = 0x25E;
                    } else if (id == 0xB) {
                        model = 0x259;
                        tint  = 0;
                    } else if (id == 0x29) {
                        model = 0x25A;
                        tint  = 0;
                    } else if (model == 0x276) {
                        model = 0x257;
                    }
                    break;
                case 1:
                    if (id == 0x49 || id == 0x69) {
                        model = 0x25D;
                    } else if (id == 0x47) {
                        model = 0x260;
                    } else if (id == 0x5E) {
                        model = 0x262;
                    } else {
                        model = 0x25C;
                    }
                    break;
                case 2:
                    if (id == 0x87) {
                        model = 0x25F;
                    } else if (id == 0xA3) {
                        model = 0x261;
                    } else {
                        model = 0x25B;
                    }
                    break;
                case 3:
                    if (model == 0x276) {
                        model = 0x1A8;
                    }
                    if (model == 0x1C1) {
                        model = 0x257;
                    }
                    break;
                case 4:
                    if (model == 0x276) {
                        model = 0x1D9;
                    }
                    break;
            }
            self->back[slot] = part_new(model);
            if (tint) {
                u16 buf[6];
                p = self->back[slot];
                func_020ab40c(p->sub, func_0201ca1c(model));
                func_0201c778(buf);
                if (info->f6 != 0xFFFF) {
                    func_0201c858(buf, 0, info->f6);
                }
                func_020ab574(self->back[slot]->sub, buf, 1);
            }
            self->back[slot]->unknown24(0x61, 1, 1, 0);
            part_set_scale_all(self->back[slot], data_020ff2d4[k]);
        }
    }
}
