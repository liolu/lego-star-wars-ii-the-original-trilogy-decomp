/* With flags bit 0 set, walks the item list of one 16x16 grid cell (list heads at grid+0x94; each cell index is the
 * x or z coordinate >> (func_0201aaec() + GRID_SHIFT), low 4 bits) and keeps the item with the lowest func_020111c4 value for
 * (item->pos - pos); that item goes to func_0206205c(item, 3, &p), with p = pos raised by 0x4000 in y. If bit 1 of
 * data_02104d90 is set or func_020867ec(&data_02104d90, 2) is nonzero, only func_02061a9c(grid, 3) runs. With no item,
 * light 3 of data_027e0874 gets colour 0 and the direction (k, k, -0.9), k = data_020ff9a8 (0.9 with 12 fraction
 * bits, the fill direction of func_020932e8 negated in z).
 *
 * Matching notes: data_027e0874 is declared as a struct (as in func_020932e8); an extern Light array forwards the
 * direction stores into the copy. The copy source is the pointer vec_init returns, which keeps the [r3] loads and
 * the pool order. */
#include "m2c.h"

enum {
    GRID_MASK    = 0xF,
    GRID_SHIFT   = 5,
    POS_Y_OFFSET = 0x4000,
    LIGHT_INDEX  = 3,
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} Color;

typedef struct {
    Vec3 dir;
    Color color;
    u16 pad;
} Light;

typedef struct {
    Light lights[4];
    u8 pad40[0x18];
} Data874;

typedef struct {
    u8 pad[0x1C];
    Vec3 pos;
} Item;

typedef struct Node {
    Item *item;
    struct Node *next;
} Node;

typedef struct {
    u8 pad[0x94];
    Node *cells[16][16];
} Grid;

typedef struct {
    s32 v;
} Fixed;

extern void func_02007f24(Fixed *out, f64 value);
extern void func_020111c4(Fixed *out, const Vec3 *v);
extern s32 func_0201aaec(void);
extern void func_02061a9c(Grid *grid, s32 mode);
extern void func_0206205c(Item *item, s32 mode, Vec3 *pos);
extern s32 func_020867ec(void *flags, s32 bit);

extern u32 data_02104d90;
extern s32 data_020ff9a8;
extern Data874 data_027e0874;

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 negp(Fixed *a) {
    s32 r    = a->v;
    Fixed *p = a; // FAKE: local copy + always-true null test keeps the add rX,sp / ldr form (see func_020ac130)
    if (p) {
        r = -p->v;
    }
    return r;
}

static inline Vec3 *vec_init(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
    return v;
}

static inline void light_set(s32 i, const Vec3 *dir, Color c) {
    data_027e0874.lights[i].color = c;
    vec_set(&data_027e0874.lights[i].dir, dir);
}

static inline int fx_lt(const Fixed *a, const Fixed *b) {
    return a->v < b->v;
}
static inline void fx_set(Fixed *d, const Fixed *s) {
    d->v = s->v;
}

void func_01ffa520(Grid *grid, Vec3 *pos, s32 flags) {
    Item *best;
    Vec3 p;
    Fixed bestDist;
    Fixed dist;
    Vec3 d;
    Node *n;
    Fixed scale;
    Vec3 dir;
    s32 zi;
    s32 xi;

    if ((data_02104d90 & 2) || func_020867ec(&data_02104d90, 2)) {
        func_02061a9c(grid, 3);
        return;
    }
    best = NULL;
    p.x  = pos->x;
    p.y  = pos->y;
    p.z  = pos->z;
    p.y += POS_Y_OFFSET;
    func_02007f24(&bestDist, 2147483647.0);
    if (flags & 1) {
        zi = p.z >> (func_0201aaec() + GRID_SHIFT);
        xi = p.x >> (func_0201aaec() + GRID_SHIFT);
        for (n = grid->cells[xi & GRID_MASK][zi & GRID_MASK]; n != NULL; n = n->next) {
            Item *item = n->item;
            vec_sub(&d, &item->pos, pos);
            func_020111c4(&dist, &d);
            if (fx_lt(&dist, &bestDist)) {
                fx_set(&bestDist, &dist);
                best = item;
            }
        }
    }
    if (best != NULL) {
        func_0206205c(best, 3, &p);
        return;
    }
    func_02007f24(&scale, 0.9);
    {
        const Vec3 *src = vec_init(&dir, data_020ff9a8, data_020ff9a8, negp(&scale));
        Color col;
        col.v = 0;
        light_set(LIGHT_INDEX, src, col);
    }
}
