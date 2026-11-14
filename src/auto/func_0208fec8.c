#include "m2c.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct Zone {
    s32 unk0;
    Vec3 min;
    Vec3 max;
    u8 pad1C[5];
    u8 disabled;
    u8 pad22[2];
} Zone;

typedef struct ZoneSet {
    Zone zones[26];
    u8 pad3A8[0x20];
    u32 count;
} ZoneSet;

typedef struct Obj {
    u8 pad0[0xb0];
    Vec3 pos;
} Obj;

/* Distance between the three-word vectors at a1 and a2 (hardware square root). */
void func_0202d884(s32 *out, s32 *a1, s32 *a2);

/* Reads a distance through a const view of the out word. */
// FAKE: identity getter, only used for the redundant re-read in func_0208fec8 (see the FAKE there)
static inline s32 dist_get(const s32 *p) {
    return *p;
}

void func_0209007c(void *self, void *event, u32 arg);

static inline s32 zone_contains(Zone *z, Vec3 *p) {
    if (p->x < z->min.x || p->x > z->max.x || p->z < z->min.z || p->z > z->max.z || p->y < z->min.y || p->y > z->max.y) {
        return 0;
    }
    return 1;
}

enum {
    NEAREST_DIST_START = 0x3e8000, /* 1000.0 in 20.12 fixed point */
};

/* If obj->pos lies inside the box of an enabled zone, passes that zone's index to
 * func_0209007c. Otherwise passes the index of the enabled zone whose box is nearest
 * to obj->pos (distance to the point clamped into the box), or self->count if none
 * is nearer than NEAREST_DIST_START. */
void func_0208fec8(ZoneSet *self, Obj *obj) {
    Zone *zone;
    s32 d;
    u32 i;
    Vec3 clamped;
    u32 bestIdx;
    u32 j;
    s32 dist;
    Zone *z;
    s32 best;

    for (i = 0, z = self->zones; i < self->count; i++, z++) {
        if (!z->disabled && zone_contains(z, &obj->pos)) {
            func_0209007c(self, obj, i);
            return;
        }
    }

    clamped.x = 0;
    clamped.y = 0;
    clamped.z = 0;
    best      = NEAREST_DIST_START;
    bestIdx   = self->count;
    for (j = 0; j < self->count; j++) {
        zone = &self->zones[j];
        if (zone->disabled) {
            continue;
        }
        if (obj->pos.x < zone->min.x) {
            clamped.x = zone->min.x;
        } else if (obj->pos.x > zone->max.x) {
            clamped.x = zone->max.x;
        } else {
            clamped.x = obj->pos.x;
        }
        if (obj->pos.y < zone->min.y) {
            clamped.y = zone->min.y;
        } else if (obj->pos.y > zone->max.y) {
            clamped.y = zone->max.y;
        } else {
            clamped.y = obj->pos.y;
        }
        if (obj->pos.z < zone->min.z) {
            clamped.z = zone->min.z;
        } else if (obj->pos.z > zone->max.z) {
            clamped.z = zone->max.z;
        } else {
            clamped.z = obj->pos.z;
        }
        func_0202d884(&d, &obj->pos.x, &clamped.x);
        // FAKE: redundant re-read: dist already holds d, but the compare below reads d again; the two reads
        // give the target's extra copy mov r1,r0 (the plain single-read form misses only that copy, judges 5)
        dist = dist_get(&d);
        if (d < best) {
            best    = dist;
            bestIdx = j;
        }
    }
    func_0209007c(self, obj, bestIdx);
}
