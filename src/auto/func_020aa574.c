#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Pt;   /* 20.12 fixed-point position */
typedef struct {
    s32 v;
} Cost;

enum {
    OBJ_POINTS = 0x21C,   /* path point list handle (caller: Obj.points) */
    OBJ_WRAPS  = 0x241    /* nonzero: indices wrap around the point list */
};

extern u8 func_02067b54(void *);
extern void func_020a9d78(Pt *, void *, u8);
extern void func_020a9dd4(Cost *, void *, Pt *, Pt *, Pt *);
extern void func_020aa794(void *, Pt *);

/* Number of points in the path list. */
static inline u32 point_count(void *self) {
    return func_02067b54(M2C_FIELD(self, void **, OBJ_POINTS));
}

static inline int cost_lt(const Cost *a, const Cost *b) {
    return a->v < b->v;
}
static inline void cost_set(Cost *d, const Cost *s) {
    d->v = s->v;
}

/* Refresh the path point index pair (x, y) (called by func_020a542c with
 * near_idx/next_idx).
 * Fetch the reference position (func_020aa794), then cost the current pair,
 * the pair advanced by one and the pair moved back by one with func_020a9dd4
 * (positions from func_020a9d78). A shifted pair is only tried when both
 * indices stay inside the list or the list wraps; out-of-range indices wrap
 * to 0 / count - 1. The pair with the lowest cost is written to
 * *out_x/*out_y (ties keep the earlier pair). */
void func_020aa574(void *self, u8 x, u8 y, u8 *out_x, u8 *out_y) {
    Pt ref_pos;
    Pt x_pos, y_pos;
    Cost best_cost;
    Cost next_cost, prev_cost;
    u32 next;
    u8 nx, ny;
    s16 px, py;

    ref_pos.x = 0;
    ref_pos.y = 0;
    ref_pos.z = 0;
    func_020aa794(self, &ref_pos);
    func_020a9d78(&x_pos, self, x);
    func_020a9d78(&y_pos, self, y);
    func_020a9dd4(&best_cost, self, &ref_pos, &x_pos, &y_pos);
    *out_x = x;
    *out_y = y;
    next   = x + 1;
    if ((next < point_count(self) && y + 1 < point_count(self)) || M2C_FIELD(self, u8 *, OBJ_WRAPS) != 0) {
        Pt nx_pos, ny_pos;
        nx = next;
        ny = y + 1;
        if (nx >= point_count(self)) {
            nx = 0;
        }
        if (ny >= point_count(self)) {
            ny = 0;
        }
        func_020a9d78(&nx_pos, self, nx);
        func_020a9d78(&ny_pos, self, ny);
        func_020a9dd4(&next_cost, self, &ref_pos, &nx_pos, &ny_pos);
        if (cost_lt(&next_cost, &best_cost)) {
            cost_set(&best_cost, &next_cost);
            *out_x = nx;
            *out_y = ny;
        }
    }
    if ((x != 0 && y != 0) || M2C_FIELD(self, u8 *, OBJ_WRAPS) != 0) {
        Pt px_pos, py_pos;
        px = x - 1;
        py = y - 1;
        if (px < 0) {
            px = point_count(self) - 1;
        }
        if (py < 0) {
            py = point_count(self) - 1;
        }
        func_020a9d78(&px_pos, self, px);
        func_020a9d78(&py_pos, self, py);
        func_020a9dd4(&prev_cost, self, &ref_pos, &px_pos, &py_pos);
        if (cost_lt(&prev_cost, &best_cost)) {
            cost_set(&best_cost, &prev_cost);
            *out_x = px;
            *out_y = py;
        }
    }
}
