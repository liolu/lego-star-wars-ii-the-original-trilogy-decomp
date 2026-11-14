#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} Ang;

extern const s16 data_020f88c4[];
extern u8 *func_020949b8(u8 *self, s32 *pos, s32 kind, u32 flag);
extern void func_020c8244(void *mtx, s16 s, s16 c);
extern void func_01ffa958(Vec3 *, const void *, const Vec3 *);

enum {
    GRID_STEP = 0x4000
};      /* 4.0 in 20.12 fixed point: spacing between grid points */
enum {
    CIRCLE_POINTS = 12
};
enum {
    CIRCLE_ANGLE_STEP = 0x1555
}; /* 0x10000 / 12: one twelfth of a turn */
enum {
    MTX33_SIZE = 0x24
};       /* 3 x 3 s32 matrix */

/* o = a + b; a and b are read as s32[3], last component first. */
static inline void vec_add(Vec3 *o, const s32 *a, const s32 *b) {
    s32 x, y, z;
    // FAKE: backward pointer walk over the vectors as s32[3] (callers cast (s32 *) &r); possible steering
    // spelling. Because the inline modifies its parameters, the inliner makes a temp for &r that LICM hoists
    // after &mtx: gives the late add r6,sp,#0x1c and the ldmib r6 / ldr ip,[r6] reads. Component or forward
    // forms fold to direct [sp,#..] reads (14-39).
    a += 2;
    b += 2;
    z    = *a-- + *b--;
    y    = *a-- + *b--;
    x    = *a + *b;
    o->x = x;
    o->y = y;
    o->z = z;
}

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

static inline Ang ang_add(const Ang *a, Ang b) {
    Ang r;
    r.v = a->v + b.v;
    return r;
}

static inline void ang_inc(Ang *a, Ang b) {
    a->v = ang_add(a, b).v;
}

/* t holds s16 (sin, cos) pairs indexed by angle >> 4. */
static inline void rot_matrix_t(void *m, const s16 *t, Ang a) {
    func_020c8244(m, t[(a.v >> 4) * 2], t[(a.v >> 4) * 2 + 1]);
}

static inline void grid_fill(u8 *self, u8 *a2, s32 n, s32 minX, s32 y, s32 maxZ) {
    s32 i;
    s32 j;
    for (i = 0; i < n; i++) {
        Vec3 p;
        p.x = minX;
        p.y = y;
        p.z = maxZ - i * GRID_STEP;
        for (j = 0; j < n; j++) {
            func_020949b8(self, (s32 *) &p, a2[4], 0);
            p.x += GRID_STEP;
        }
    }
}

/* int (not s32) position params: with them the minX/y/maxZ spill slots come out in the target order */
static inline void grid_border(u8 *self, u8 *a2, s32 n, int minX, int y, int maxZ) {
    s32 i;
    s32 j;
    for (i = 0; i < n; i++) {
        Vec3 p;
        p.x = minX;
        p.y = y;
        p.z = maxZ - i * GRID_STEP;
        for (j = 0; j < n; j++) {
            if (j == 0 || j == n - 1 || i == 0 || i == n - 1) {
                func_020949b8(self, (s32 *) &p, a2[4], 0);
            }
            p.x += GRID_STEP;
        }
    }
}

static inline void ang_step(Ang *a, const Ang *s) {
    ang_inc(a, *s);
}

static inline void circle(u8 *self, Vec3 *pos, u8 *a2, s32 count) {
    const s16 *tab;
    s32 i;
    Ang step;
    Ang ang;
    Vec3 r;
    Vec3 p;
    u8 mtx[MTX33_SIZE];
    Vec3 radius;

    tab      = data_020f88c4;
    radius.x = 0;
    radius.y = 0;
    radius.z = count << 12;
    ang.v    = 0;
    for (i = 0; i < CIRCLE_POINTS; i++) {
        step.v = CIRCLE_ANGLE_STEP;
        rot_matrix_t(mtx, tab, ang);
        func_01ffa958(&r, mtx, &radius);
        vec_add(&p, (s32 *) &r, (s32 *) pos);
        func_020949b8(self, (s32 *) &p, a2[4], 0);
        ang_step(&ang, &step);
    }
}

/* Calls func_020949b8(self, point, a2[4], 0) for each point of a pattern around pos.
 * a2[2] selects the pattern, a2[3] is its size:
 *   1: n x n grid, GRID_STEP apart (n = size rounded up to even), from x = pos->x - off,
 *      z = pos->z + off with off = (n / 2) * GRID_STEP; columns go +x, rows go -z;
 *   2: only the outer ring of that grid;
 *   3: CIRCLE_POINTS points, (0, 0, size) rotated by func_020c8244 matrices, added to pos. */
void func_02095bd0(u8 *self, Vec3 *pos, u8 *a2) {
    s32 n;
    s32 count;
    s32 off;
    int minX; /* the int/s32 mix here and in grid_border sets the spill-slot order */
    s32 y;
    int maxZ;
    s32 mode;

    count = a2[3];
    n     = count;
    if (count & 1) {
        n = count + 1;
    }
    off  = fx_mul((n >> 1) << 12, GRID_STEP);
    minX = pos->x;
    maxZ = pos->z;
    minX -= off;
    maxZ += off;
    y    = pos->y;
    mode = a2[2];

    if (mode == 1) {
        grid_fill(self, a2, n, minX, y, maxZ);
    } else if (mode == 2) {
        grid_border(self, a2, n, minX, y, maxZ);
    } else if (mode == 3) {
        circle(self, pos, a2, count);
    }
}
