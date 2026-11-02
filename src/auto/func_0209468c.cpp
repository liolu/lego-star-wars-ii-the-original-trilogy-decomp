#include "m2c.h"

// Partial interface: unknown virtuals keep their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
// Container of the spawner's pending path points, embedded at spawner + 0x70.
// Slot 0x8 adds a point (func_02094930). Slot 0xc takes a point out of the
// list: the drain loop below only ends because each call unlinks the head.
class PointList {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void remove(void *point);
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
};

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

// One fx32 (20.12 fixed-point) value passed by value.
typedef struct Fixed {
    s32 v;
} Fixed;

// Spawn record attached to a pending point (the `cmd` argument of func_02094930).
typedef struct SpawnCmd {
    u8 pad0;
    u8 link_id; // id of the pending point this one connects to; 0 = none
    u8 pad2;
    u8 count;   // objects to place along the segment
    u8 kind;    // object kind passed to func_020949b8
} SpawnCmd;

// Pending path point (0x14 bytes, built by func_0209435c). Its u16 id at +0x10
// is what func_020948f4 searches for.
typedef struct PathPoint {
    Vec3 pos;
    SpawnCmd *cmd;
} PathPoint;

typedef struct Node {
    PathPoint *point;
    struct Node *next;
} Node;

// Data view of the same PointList object.
struct PointListData {
    void *vtable;
    Node *head;
    u32 unknown08;
    u16 count;
};

// Part of the large global at data_02104d90 (func_020953b0 uses the same
// fields): a u16 cursor at +0x2970 and, at +0x2972, the first index in the
// shared 1024-bit table (+0x2a08) reserved for each value of func_020917b4.
struct Tables {
    u8 pad0[0x2970];
    u16 next_base;
    u16 base[0x4b];
};

enum {
    FX32_SHIFT       = 12,
    BASE_UNASSIGNED  = 0xffff,
    SPAWNER_IDS_USED = 0xf0, // u16: persistent ids handed out so far
    SPAWNER_ID_MODE  = 0xf8,  // u8: nonzero while spawned objects get persistent ids
};

extern "C" s32 FX_Div(s32, s32);
extern "C" void *func_020948c0(void *);
extern "C" Vec3 *func_020948f4(void *, s32);
extern "C" void func_020949b8(void *, Vec3 *, s32, s32);
extern "C" void func_0205e84c(void *);
extern "C" u8 func_020917b4(void *);
extern "C" u8 data_0210930c[];
extern "C" Tables data_02104d90;

static inline s32 fx_div(s32 a, Fixed b) {
    return FX_Div(a, b.v);
}

static inline void vec_from_zyx(Vec3 *v, s32 z, s32 y, s32 x) {
    v->x = x;
    v->y = y;
    v->z = z;
}

/* Flush the spawner's pending path points. self is the spawner object at
 * manager + 0xa4 (func_02059590 passes data_021093b0 = data_0210930c + 0xa4).
 * While some pending point links to another one (func_020948c0), look up the
 * linked end point by id (func_020948f4) and place cmd->count objects of
 * cmd->kind from the start point toward it, one step of (end - start) / count
 * apart, so the last object stops one step short of the end. The end point (if
 * found) and the start point are then removed from the list and freed. Points
 * still pending afterwards (no link, not used as an end) are removed and freed
 * too, then the list's slot 0x18 method runs.
 *
 * assign_ids != 0 (func_02090f3c; func_02094ca0 and func_02059590 pass 0)
 * turns on id mode for the flush: func_020949b8 numbers each object with the
 * counter at self + 0xf0 and skips (but still counts) ids already marked in the
 * shared bit table (func_020953b0 marks the id of an object whose life runs
 * out). Afterwards, if the current func_020917b4 index has no range in that
 * table yet, reserve one at the shared cursor and advance it by the ids used.
 */
extern "C" void func_0209468c(u8 *self, u8 assign_ids) {
    PointList *points          = (PointList *) (self + 0x70);
    PointListData *points_data = (PointListData *) (self + 0x70);
    PathPoint *start;
    s32 i;
    if (assign_ids != 0) {
        self[SPAWNER_ID_MODE] = 1;
    }
    while ((start = (PathPoint *) func_020948c0(self)) != NULL) {
        Vec3 pos;
        Vec3 *end;
        SpawnCmd *cmd;
        s32 kind, count;
        pos.x = start->pos.x;
        pos.y = start->pos.y;
        pos.z = start->pos.z;
        cmd   = start->cmd;
        count = cmd->count;
        kind  = cmd->kind;
        end   = func_020948f4(self, cmd->link_id);
        if (end != NULL) {
            Vec3 step;
            Vec3 delta;
            s32 scale;
            scale   = count << FX32_SHIFT; // count as fx32
            delta.z = end->z - pos.z;
            delta.y = end->y - pos.y;
            delta.x = end->x - pos.x;
            // FAKE: address-taken scale makes each by-value Fixed copy a real stack slot
            vec_from_zyx(&step, fx_div(delta.z, *(Fixed *) &scale), fx_div(delta.y, *(Fixed *) &scale),
                         fx_div(delta.x, *(Fixed *) &scale));
            for (i = 0; i < count; i++) {
                func_020949b8(self, &pos, kind, 1);
                pos.x += step.x;
                pos.y += step.y;
                pos.z += step.z;
            }
            points->remove(end);
            if (end != NULL) {
                func_0205e84c(end);
            }
        }
        points->remove(start);
        if (start != NULL) {
            func_0205e84c(start);
        }
    }
    if (points_data->count != 0 && points_data->head != NULL) {
        do {
            PathPoint *point = points_data->head->point;
            points->remove(point);
            if (point != NULL) {
                func_0205e84c(point);
            }
        } while (*(Node **) (self + 0x74) !=
                 NULL); // FAKE: re-reads head (via raw field) so the loop test is not CSEd with the body load
    }
    points->unknown18();
    if (assign_ids != 0) {
        u16 ids_used;
        u8 index;
        self[SPAWNER_ID_MODE] = 0;
        ids_used              = *(u16 *) (self + SPAWNER_IDS_USED);
        index                 = func_020917b4(data_0210930c);
        if (data_02104d90.base[index] == BASE_UNASSIGNED) {
            data_02104d90.base[index] = data_02104d90.next_base;
            data_02104d90.next_base += ids_used;
        }
    }
}
