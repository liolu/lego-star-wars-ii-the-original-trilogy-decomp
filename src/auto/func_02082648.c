/* Reconstructed from USA ARM9 at 0x02082648 (1344 bytes).
 *
 * Places the parts of a 4 x 2 grid of slots (two parts per slot: back[] and
 * front[]) plus two extra parts.  Positions are 20.12 fixed point (to_fx).
 * The grid row of slot (i, j) is (i + tab[sel]) % 4, so the table entry
 * picked by sel rotates the rows.  When the grid index (unwrapped row * 2 + j)
 * equals the selected id and f124 is 0, both parts of the slot move to the
 * two 16-bit values at 0x2A / 0x2C of data_021044a4 (the second taken as
 * 0xB0 - value); otherwise they go to their grid cell, whose y is shifted by
 * f124 (and wrapped by 0x94 for the last row when f124 is negative).  The
 * extra parts use the ids 0x100 / 0x101 the same way, else fixed positions.
 * part_set_pos only writes, and sets the dirty flag, when the position
 * changes.
 */
#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Part {
    u8 pad0[0xB0];
    Vec3 pos;
    u8 pad1[0xD1 - 0xBC];
    u8 posDirty;
} Part;

typedef struct Owner {
    u8 pad0[0x28];
    Part *front[8];
    u8 pad1[0xC0 - 0x48];
    Part *back[8];
    u8 pad2[0xE4 - 0xE0];
    Part *extraA;
    Part *extraB;
    u8 pad3[0x108 - 0xEC];
    s32 sel;
    s32 tab[6];
    s32 f124;
    s32 selected; /* grid index, or 0x100 / 0x101 for the extras */
} Owner;

typedef struct Data {
    u8 pad[0x2A];
    u16 f2a;
    u16 f2c;
} Data;

extern Data data_021044a4;

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

enum {
    FX_SHIFT      = 12,
    GRID_ROWS     = 4,
    GRID_COLS     = 2,
    GRID_X0       = 0x3C,
    GRID_DX       = 0x28,
    GRID_Y0       = 0x66,
    GRID_DY       = 0x25,
    GRID_LAST_ROW = 3,
    GRID_WRAP_Y   = 0x94,
    DATA_Y_FLIP   = 0xB0,
    SEL_BACK_Z    = -0x64000,
    SEL_FRONT_Z   = -0x5A000,
    GRID_BACK_Z   = -0xC8000,
    GRID_FRONT_Z  = -0xBE000,
    EXTRA_A_ID    = 0x100,
    EXTRA_B_ID    = 0x101,
    EXTRA_A_X     = 0xE8000,
    EXTRA_A_Y     = 0x70000,
    EXTRA_B_X     = 0xE4000,
    EXTRA_B_Y     = 0x3E000,
    EXTRA_REST_Z  = -0xC8000,
};

static inline s32 to_fx(s32 v) {
    return v << FX_SHIFT;
}

static inline Vec3 *vec_make(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
    return v;
}

static inline void part_set_pos(Part *p, const Vec3 *v) {
    if (vec_ne(&p->pos, v)) {
        vec_set(&p->pos, v);
        p->posDirty = 1;
    }
}

void func_02082648(Owner *self) {
    s32 j, i;

    for (i = 0; i < GRID_ROWS; i++) {
        for (j = 0; j < GRID_COLS; j++) {
            int row  = i + self->tab[self->sel];
            int slot = j + (row % GRID_ROWS) * GRID_COLS;

            if (self->back[slot] == NULL) {
                continue;
            }
            if (self->selected == row * GRID_COLS + j && self->f124 == 0) {
                Vec3 a;
                Vec3 b;
                part_set_pos(self->back[slot],
                             vec_make(&a, to_fx(data_021044a4.f2a), to_fx(DATA_Y_FLIP - data_021044a4.f2c), SEL_BACK_Z));
                if (self->front[slot] == NULL) {
                    continue;
                }
                part_set_pos(self->front[slot],
                             vec_make(&b, to_fx(data_021044a4.f2a), to_fx(DATA_Y_FLIP - data_021044a4.f2c), SEL_FRONT_Z));
            } else {
                Vec3 c;
                Vec3 d;
                s32 y = GRID_Y0 - i * GRID_DY + self->f124;
                if (i == GRID_LAST_ROW && self->f124 < 0) {
                    y += GRID_WRAP_Y;
                }
                part_set_pos(self->back[slot], vec_make(&c, to_fx(GRID_X0 + j * GRID_DX), to_fx(y), GRID_BACK_Z));
                if (self->front[slot] == NULL) {
                    continue;
                }
                part_set_pos(self->front[slot], vec_make(&d, to_fx(GRID_X0 + j * GRID_DX), to_fx(y), GRID_FRONT_Z));
            }
        }
    }

    if (self->extraA != NULL) {
        if (self->selected == EXTRA_A_ID) {
            Vec3 e;
            part_set_pos(self->extraA,
                         vec_make(&e, to_fx(data_021044a4.f2a), to_fx(DATA_Y_FLIP - data_021044a4.f2c), SEL_BACK_Z));
        } else {
            Vec3 f;
            part_set_pos(self->extraA, vec_make(&f, EXTRA_A_X, EXTRA_A_Y, EXTRA_REST_Z));
        }
    }
    if (self->extraB != NULL) {
        if (self->selected == EXTRA_B_ID) {
            Vec3 g;
            part_set_pos(self->extraB,
                         vec_make(&g, to_fx(data_021044a4.f2a), to_fx(DATA_Y_FLIP - data_021044a4.f2c), SEL_BACK_Z));
        } else {
            Vec3 h;
            part_set_pos(self->extraB, vec_make(&h, EXTRA_B_X, EXTRA_B_Y, EXTRA_REST_Z));
        }
    }
}
