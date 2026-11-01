/* Reconstructed from USA ARM9 at 0x020910bc (596 bytes).
 * Builds the node list of the object at self->0x3c from the groups owned by self->0x68:
 *   1. sum the entry counts of every group and reserve that many (+1) slots in a newly
 *      allocated vector (func_02093d88 grows the vector's buffer);
 *   2. reset the record table at self+0x61c (func_02095f28);
 *   3. for every entry: allocate a 0x3c-byte node at the entry's s16 position converted
 *      to 20.12 fixed point, append (base + offset, table byte) pairs taken from the entry and the
 *      byte table data_020f50dc, add records to the self+0x61c table for non-zero entry
 *      bytes, then store the node at running index base + j (func_02075064).
 */
#include "m2c.h"

enum {
    FX_SHIFT        = 12,
    VEC_ALLOC_SIZE  = 0x14,
    NODE_ALLOC_SIZE = 0x3c,
    TABLE_OFFSET    = 0x61c,
    NO_SLOT         = 0xff,
    IDX_COUNT       = 4,
    SUB_COUNT       = 3,
};

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u32 *data;
    u32 size;
    u32 capacity;
    u32 pad;
    u32 extra;
} Vec;

typedef struct {
    s16 x, y, z;
    u8 count;
    u8 scale;
    u8 flag;
    u8 idx[4];
    u8 sub[3];
    u8 off[4];
} Entry;

typedef struct {
    u8 a;
    u8 b;
} Pair;

typedef struct {
    u8 n;
    Pair p[1];
} Obj;

typedef struct {
    u8 pad[8];
    u8 n;
} Item;

extern const u8 data_020f50dc[];

void *func_0205e80c(s32, s32, s32);
Entry *func_02067b7c(void *arg0, u32 arg1);
Obj *func_02074c3c(void *a0, Vec3 *a1, Fx a2);
void func_02075064(Vec *v, u32 value, u32 idx);
u16 func_02090708(void *arg0);
Item *func_02090718(void *arg0, s32 arg1);
void func_02093d88(Vec *self, u32 n);
void func_02095f28(void *arg0);
void func_02095fc4(void *a0, s32 a1, s32 a2, s32 a3);

void func_020910bc(void *self) {
    u32 group_count;
    u32 entry_total;
    Item *group;
    u32 i;
    u32 base;
    Vec *vec;

    group_count = func_02090708(M2C_FIELD(self, void **, 0x68));
    if (group_count == 0) {
        return;
    }
    entry_total = 0;
    for (i = 0; i < group_count; i++) {
        group = func_02090718(M2C_FIELD(self, void **, 0x68), i);
        entry_total += group->n;
    }
    vec = func_0205e80c(VEC_ALLOC_SIZE, 3, 4);
    if (vec != NULL) {
        vec->data     = 0;
        vec->size     = 0;
        vec->capacity = 0;
        vec->extra    = 0;
    }
    M2C_FIELD(self, Vec **, 0x3c) = vec;
    if (entry_total + 1 > vec->capacity) {
        func_02093d88(vec, entry_total + 1);
    }
    func_02095f28((u8 *) self + TABLE_OFFSET);
    base = 0;
    for (i = 0; i < group_count; i++) {
        u32 j;
        group = func_02090718(M2C_FIELD(self, void **, 0x68), i);
        for (j = 0; j < group->n; j++) {
            Entry *entry = func_02067b7c(group, j);
            Vec3 pos;
            u32 k;
            Obj *node;

            {
                s32 z = entry->z << FX_SHIFT;
                s32 y = entry->y << FX_SHIFT;
                s32 x = entry->x << FX_SHIFT;
                pos.x = x;
                pos.y = y;
                pos.z = z;
            }
            node = func_0205e80c(NODE_ALLOC_SIZE, 3, 4);
            if (node != NULL) {
                Fx s;
                s.v  = entry->scale << FX_SHIFT;
                node = func_02074c3c(node, &pos, s);
            }
            if (entry->flag != 0) {
                func_02095fc4((u8 *) self + TABLE_OFFSET, entry->flag, j, NO_SLOT);
            }
            for (k = 0; k < entry->count; k++) {
                u32 t;
                u8 c;
                t = 0;
                if (k < IDX_COUNT) {
                    t = entry->idx[k];
                }
                c                  = data_020f50dc[t];
                node->p[node->n].a = base + entry->off[k];
                node->p[node->n].b = c;
                node->n++;
                if (k < SUB_COUNT && entry->sub[k] != 0) {
                    func_02095fc4((u8 *) self + TABLE_OFFSET, entry->sub[k], j, k);
                }
            }
            func_02075064(M2C_FIELD(self, Vec **, 0x3c), (u32) node, base + j);
        }
        base += group->n;
    }
}
