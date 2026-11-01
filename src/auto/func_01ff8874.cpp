#include "m2c.h"

// Query region filled by the shape's slot 0x04 virtual and handed to the
// cell iterator (six words, zeroed before the call).
typedef struct Box {
    s32 v[6];
} Box;

// Cell iterator state used by func_020555c4/02055630/02055660/02055684.
typedef struct Iter {
    s32 w[6];
} Iter;

// Argument block passed to the per-object callbacks: the partner object.
typedef struct Hit {
    void *obj;
    s32 pad;
} Hit;

// Singly linked object list node.
typedef struct Link {
    void *obj;
    struct Link *next;
} Link;

typedef void (*HitFn)(void *self, Hit *hit);

// One 0x38-byte entry of the global table; word 1 is its object list.
typedef struct Slot {
    s32 unk0;
    Link *head;
    u8 pad[0x30];
} Slot;

extern "C" {
extern s32 data_027e02a4;
extern s32 data_021019d8;
extern Slot data_02101a64[];
extern M2C_UNK data_021094ac;
extern u32 data_027e0174[];
extern u32 SDK_AUTOLOAD_DTCM_START[];
extern u32 data_027e00f8[];

s32 func_01ff84bc(void *a, void *b);
void *func_020555c4(void *arg0, s32 arg1, void *arg2);
void func_02055630(void *arg0);
s32 func_02055660(const void *object);
s32 func_02055684(void *arg0);
}

enum {
    QUERY_FLAGS_REQUIRED = 0x100000, // obj+8: object must have this bit
    FLAGS_EXCLUDED       = 0x200002,       // obj+8: any of these bits skips the object
    OTHER_FLAGS_ANY      = 0x100004,      // obj+8: partner needs one of these bits
};

class Shape {
public:
    virtual void unknown00();
    virtual void getBounds(Box *box); // slot 0x04
};

// For every object in the current table entry's list (index data_027e02a4)
// whose flags qualify and whose shape (+0x24) is present and enabled (+0x26):
// fill a Box through the shape, bump the global visit stamp (data_021019d8)
// and walk every partner object returned by the cell iterator for that box.
// Each partner is visited once per query (its +0x28 takes the stamp). The
// partner's +0x24 part is tested against the query shape (func_01ff84bc)
// when the mask table data_027e0174[obj type] has the partner's type bit
// (+0x84), and then the query's +0x1c callback gets the partner. The
// partner's +0x20 part, when it passes func_01ff84bc, fires the query's
// +0x18 callback (mask SDK_AUTOLOAD_DTCM_START[obj type]) and the partner's
// +0x14 callback with the query object (mask data_027e00f8[partner type]).
// The final stamp is written back to data_021019d8.
extern "C" void func_01ff8874(void) {
    Hit pair_hit;
    Hit hit;
    Box box;
    Iter it;
    s32 stamp;
    Link *link;
    Link *node;
    void *obj;
    Shape *shape;
    void *other;
    void *part;
    s32 *zp;

    {
        Slot *slot = &data_02101a64[data_027e02a4];
        link       = slot->head;
    }
    stamp = data_021019d8;
    while (link != NULL) {
        obj  = link->obj;
        link = link->next;
        if (!(M2C_FIELD(obj, s32 *, 8) & QUERY_FLAGS_REQUIRED) || (M2C_FIELD(obj, u32 *, 8) & FLAGS_EXCLUDED)) {
            continue;
        }
        shape = M2C_FIELD(obj, Shape **, 0x24);
        if (shape == NULL || M2C_FIELD(shape, u8 *, 0x26) == 0) {
            continue;
        }
        zp = box.v;
        do {
            *zp++ = 0;
        } while (zp < box.v + 6);
        shape->getBounds(&box);
        stamp++;
        func_020555c4(&it, (s32) &data_021094ac, &box);
        while (!func_02055684(&it)) {
            for (node = (Link *) func_02055660(&it); node != NULL; node = node->next) {
                other = node->obj;
                if (!(M2C_FIELD(other, u32 *, 8) & OTHER_FLAGS_ANY)) {
                    continue;
                }
                if (M2C_FIELD(other, u32 *, 8) & FLAGS_EXCLUDED) {
                    continue;
                }
                if (obj == other || M2C_FIELD(other, s32 *, 0x28) == stamp) {
                    continue;
                }
                M2C_FIELD(other, s32 *, 0x28) = stamp;
                part                          = M2C_FIELD(other, void **, 0x24);
                if (part != NULL && M2C_FIELD(part, u8 *, 0x26) != 0 &&
                    (data_027e0174[M2C_FIELD(obj, s32 *, 0x84)] & (1 << M2C_FIELD(other, s32 *, 0x84))) &&
                    func_01ff84bc(shape, part) != 0) {
                    hit.obj = other;
                    M2C_FIELD(obj, HitFn *, 0x1c)(obj, &hit);
                }
                part = M2C_FIELD(other, void **, 0x20);
                if (part != NULL && M2C_FIELD(part, u8 *, 0x26) != 0 && func_01ff84bc(shape, part) != 0) {
                    if (SDK_AUTOLOAD_DTCM_START[M2C_FIELD(obj, s32 *, 0x84)] & (1 << M2C_FIELD(other, s32 *, 0x84))) {
                        pair_hit.obj = other;
                        M2C_FIELD(obj, HitFn *, 0x18)(obj, &pair_hit);
                    }
                    if (data_027e00f8[M2C_FIELD(other, s32 *, 0x84)] & (1 << M2C_FIELD(obj, s32 *, 0x84))) {
                        pair_hit.obj = obj;
                        M2C_FIELD(other, HitFn *, 0x14)(other, &pair_hit);
                    }
                }
            }
            func_02055630(&it);
        }
    }
    data_021019d8 = stamp;
}
