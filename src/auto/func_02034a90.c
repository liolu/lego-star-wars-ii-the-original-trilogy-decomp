/* Reconstructed from USA ARM9 at 0x02034a90 (320 bytes).
 *
 * Scans the entity list of the current player slot (data_021019f4 holds
 * 0x38-byte slot records, the list head is at +4) for a kind 4 entity with
 * no 0x802 flag bits and sub id 0x2b.  Candidates whose scaled squared
 * distance (positions >> 12) from the controller's own entity is below
 * 22500 and whose heading (func_020985d8 on the x/z difference) is within
 * 0x1555 of the entity yaw are handed to func_02025168 together with the
 * controller's own entity (stored by the callee at +0x238); on success the
 * candidate is remembered at +0x60.
 *
 * The difference vector, its x/z copy and the three halfwords are stack
 * aggregates of single-field structs, so the stores stay as in the original.
 */
#include "m2c.h"

typedef struct {
    u16 v;
} HalfVal;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx qx;
    Fx qz;
    Fx dx;
    Fx dy;
    Fx dz;
} Loc;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} V3;

typedef struct Ent {
    u8 pad0[8];
    u32 flags;       /* +0x08 */
    u16 kind;        /* +0x0c */
    u8 pad0e[0xa2];
    s32 x;           /* +0xb0 */
    s32 y;
    s32 z;
    u8 padbc[0xe];
    HalfVal yaw;     /* +0xca */
    u8 padcc[0x146];
    u16 sub;         /* +0x212 */
} Ent;

typedef struct Node {
    Ent *ent;
    struct Node *next;
} Node;

typedef struct {
    u8 pad0[0x38];
} Slot;

typedef struct {
    u8 pad0[4];
    Ent *self;
    u8 pad8[0x58];
    Ent *found;
} Ctl;

extern Slot data_021019f4[];
extern s32 data_027e02a4;
extern s16 func_020985d8(s32, s32);
extern s32 func_02025168(Ent *, Ent *);

void func_02034a90(Ctl *ctl) {
    Node *n;
    Ent *e;
    Loc l;
    HalfVal s4;
    HalfVal s2;
    HalfVal s0;
    u16 d;
    s32 dd;
    s32 vx, vy, vz;

    n = *(Node **) ((u8 *) &data_021019f4[data_027e02a4] + 4);
    if (n == NULL) {
        return;
    }
    do {
        e = n->ent;
        if (e->kind == 4 && (e->flags & 0x802) == 0 && e->sub == 0x2b) {
            vz     = e->z - ctl->self->z;
            vy     = e->y - ctl->self->y;
            vx     = e->x - ctl->self->x;
            l.dx.v = vx;
            l.dy.v = vy;
            l.dz.v = vz;
            if (((vx >> 12) * (vx >> 12) + (vy >> 12) * (vy >> 12) + (vz >> 12) * (vz >> 12)) < 22500) {
                l.qz = l.dz;
                l.qx = l.dx;
                s4.v = func_020985d8(l.dx.v, l.dz.v);
                s0   = s4;
                s2   = ctl->self->yaw;
                d    = s2.v - s0.v;
                dd   = (s16) d;
                if (dd < 0) {
                    dd = -dd;
                }
                if ((u16) dd < 0x1555) {
                    if (func_02025168(e, ctl->self)) {
                        ctl->found = e;
                    }
                    return;
                }
            }
        }
        n = n->next;
    } while (n != NULL);
}
