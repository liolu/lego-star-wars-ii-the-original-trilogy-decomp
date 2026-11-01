#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 min[3];
    s32 max[3];
} Box;

typedef struct {
    u16 *idx;
    s32 dist;
} Res;

typedef struct {
    u8 pad[0x10];
    s32 off;
} Table;

typedef struct {
    s32 f0;
    Table *tab;
} Model;

typedef struct {
    u8 pad[0x26];
    u8 active;
} Node;

typedef struct {
    s32 w;
    Vec3 v;
} Hit;

typedef struct {
    s32 w;
    Vec3 v;
    s32 dist;
    u16 *idx;
} Out;

enum {
    MAX_HIT_DIST = 0x2000, /* callers preset out->dist to this "nothing found" value */
    ENTRY_SIZE   = 12,     /* stride of the table entries indexed by *r.idx */
};

Res func_01ff9188(s32, Box *, Vec3 *);
void func_01ff9f20(void *, Node *, Vec3 *, Out *);
void func_0204c7b0(s32 *dest, s32 *src);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

/* ITCM query helper. Asks func_01ff9188 for the nearest entry of the model table
 * (arg4) against box/pos. When its distance is below MAX_HIT_DIST, the indexed
 * table entry is expanded by func_0204c7b0 and stored in *out together with the
 * distance and the index pointer. Otherwise, if node is set and its byte at 0x26
 * is non-zero, the query is handed on to func_01ff9f20 with the same out record.
 */
void func_01ffa338(void *obj, Node *node, Box *box, Vec3 *pos, s32 arg4, Out *out) {
    Res r;
    Hit h;
    Table *t;

    r = func_01ff9188(arg4, box, pos);
    if (r.dist < MAX_HIT_DIST) {
        t = ((Model *) arg4)->tab;
        func_0204c7b0(&h.w, (s32 *) ((u8 *) t + t->off + *r.idx * ENTRY_SIZE));
        out->w = h.w;
        vec_set(&out->v, &h.v);
        out->dist = r.dist;
        out->idx  = r.idx;
        return;
    }
    if (node != NULL && node->active != 0) {
        func_01ff9f20(obj, node, pos, out);
    }
}
