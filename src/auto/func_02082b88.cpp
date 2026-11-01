#include "m2c.h"

typedef struct {
    u8 f00;
    u8 f01;
    u8 f02;
    u8 f03;
} Byte4;

typedef struct {
    s16 f00;
    s16 f02;
} ShortPair;

typedef struct {
    u8 f00[8];
} Byte8;

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;

    Vec3() {}
    Vec3(s32 vx, s32 vy, s32 vz) {
        x = vx;
        y = vy;
        z = vz;
    }
};

typedef struct {
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

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
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

class Model02082b88 {
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
    virtual void unknown24(s32, s32, s32, s32);

    u8 pad_04[0xac];
    Vec3 pos;
    Vec3 scale;
    u8 pad_c8[9];
    u8 dirty;
    u8 pad_d2[6];
    u8 fd8[1];

    void SetPos(const Vec3 &v) {
        if (vec_ne(&pos, &v)) {
            vec_set(&pos, &v);
            dirty = 1;
        }
    }
    void SetScale(s32 s) {
        SetScale(Vec3(s, s, s));
    }
    void SetScale(const Vec3 &v) {
        if (vec_ne(&scale, &v)) {
            vec_set(&scale, &v);
            dirty = 1;
        }
    }
};

typedef struct Info {
    u8 pad00[4];
    u16 f04;
    u16 f06;
} Info;

typedef struct Self {
    u8 pad00[0x50];
    u8 f50[0xE4 - 0x50];
    Model02082b88 *model;
    u8 padE8[0xEC - 0xE8];
    s32 cur[17];
    u8 dirty;
    u8 pad131[3];
    u8 *tables[5];
    s32 count[5];
} Self;

extern "C" {
void func_02054c80(void *, void *, u32, u32, u32, u32, u32, u32, ShortPair *, Byte4);
Info *func_0208dad8(s32);
void *func_0205e80c(s32, s32, s32);
Model02082b88 *func_0200ff50(void *, s32, s32, s32, Cfg *);
u16 func_0201ca1c(u16);
s32 func_020ab40c(void *, u16);
void func_0201c778(u16 *);
void func_0201c858(u16 *, s32, u16);
s32 func_020ab574(void *, u16 *, s32);
}

static inline Model02082b88 *model_new(void *mem, u16 id) {
    if (mem != (void *) 0) {
        Cfg cfg;
        cfg.pos.x   = 0;
        cfg.pos.y   = 0;
        cfg.pos.z   = 0;
        cfg.scale.x = 0x1000;
        cfg.scale.y = 0x1000;
        cfg.scale.z = 0x1000;
        cfg.r0      = 0;
        cfg.r1      = 0;
        cfg.r2      = 0;
        cfg.f44     = 1;
        cfg.f45     = 1;
        mem         = func_0200ff50(mem, 4, id, 1, &cfg);
    }
    return (Model02082b88 *) mem;
}

/*
 * Selects entry idx for one of self's slots. idx at or past count[slot] means "none" (-1).
 * Slots other than 4 store the index (none becomes 0) and set self->dirty when it changes.
 * Slot 4 (only when the index changes): sets dirty, calls the current model's slot-0x04
 * virtual (no args) and clears self->model, then calls func_02054c80 on self->f50 (first
 * Byte4 field 0 for none, 4 otherwise). For a real entry it looks up the table byte via
 * func_0208dad8, remaps id 0x276 to 0x1d9, allocates 0x1bc bytes (func_0205e80c) and
 * constructs the model with func_0200ff50 (zero position, 0x1000 scale config) into self->model,
 * applies func_0201ca1c(id) and the optional f06 value to its fd8 member, calls its
 * slot-0x24 virtual (0x62, 1, 1, 0) and sets a fixed position and a uniform 0x6000 scale.
 */
extern "C" void func_02082b88(Self *self, s32 slot, s32 idx) {
    enum {
        MODEL_SLOT = 4,
        NO_ENTRY   = -1,
        MODEL_SIZE = 0x1bc,
        NO_VALUE   = 0xffff,
    };
    if (idx >= self->count[slot]) {
        idx = NO_ENTRY;
    }
    if (slot == MODEL_SLOT) {
        Model02082b88 *model;
        void *mem;
        Info *info;
        u16 id;
        u16 values[6];

        if (idx == self->cur[slot]) {
            return;
        }
        self->cur[slot] = idx;
        self->dirty     = 1;
        if (self->model != (Model02082b88 *) 0) {
            self->model->unknown04();
        }
        self->model = (Model02082b88 *) 0;
        if (idx == NO_ENTRY) {
            Byte8 out;
            ShortPair pair;
            Byte4 bytes;

            bytes.f00 = 0;
            bytes.f01 = 0x24;
            bytes.f02 = 4;
            bytes.f03 = 6;
            pair.f00  = 0x1B;
            pair.f02  = 5;
            func_02054c80(&out, self->f50, 0x59E, 0x10D, 1, 0x10, 0, 0, &pair, bytes);
            return;
        }
        {
            Byte8 out;
            ShortPair pair;
            Byte4 bytes;

            bytes.f00 = 4;
            bytes.f01 = 0x24;
            bytes.f02 = 4;
            bytes.f03 = 6;
            pair.f00  = 0x1B;
            pair.f02  = 5;
            func_02054c80(&out, self->f50, 0x59E, 0x10D, 1, 0x10, 0, 0, &pair, bytes);
        }
        info = func_0208dad8(self->tables[slot][idx]);
        id   = info->f04;
        if (id == 0x276) {
            id = 0x1D9;
        }
        mem         = func_0205e80c(MODEL_SIZE, 3, 4);
        self->model = model_new(mem, id);
        model       = self->model;
        func_020ab40c(model->fd8, func_0201ca1c(id));
        func_0201c778(values);
        if (info->f06 != NO_VALUE) {
            func_0201c858(values, 0, info->f06);
        }
        func_020ab574(self->model->fd8, values, 1);
        self->model->unknown24(0x62, 1, 1, 0);
        self->model->SetPos(Vec3(0xE8000, 0x70000, -0xC8000));
        self->model->SetScale(0x6000);
    } else {
        if (idx == NO_ENTRY) {
            idx = 0;
        }
        if (idx != self->cur[slot]) {
            self->cur[slot] = idx;
            self->dirty     = 1;
        }
    }
}
