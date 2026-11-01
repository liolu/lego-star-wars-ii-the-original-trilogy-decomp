#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

// Object held in Task::slots; its position follows an owner item each frame.
typedef struct Item {
    u8 pad0[0x24];
    Vec3 pos;
    u8 pad30[0x15];
    u8 dirty; // Set to 1 whenever pos is rewritten here.
} Item;

struct Anchor {
    u8 pad0[4];
    Vec3 pos;
};
typedef struct Anchor Anchor;

// Declarations only: nine unknown virtuals keep notify() at vtable slot 0x24.
class Owner {
public:
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void notify(s32 id, s32 a, s32 b, s32 c);
    u8 pad4[0x28];
    struct Anchor *items[3];
    u8 pad38[8];
    u8 flag40;
    u8 pad41[0x23];
    s32 timer;
    u8 pad68[0x4c];
    s32 now;
    u8 padb8[0x516];
    u16 buttons;
};

typedef struct Task {
    u8 pad0[4];
    Owner *owner;
    u8 pad8[4];
    u8 flags;
    u8 state;
    u8 pade[2];
    s32 arg10;
    s32 count;
    s32 start;
    Item *slots[2];
    void *handle;
} Task;

// Converts a double to 20.12 fixed point (value * 4096).
extern "C" void func_02007f24(s32 *, f64);
extern "C" void func_02047b08(Task *);
extern "C" void func_02047b94(Task *, s32);
extern "C" void func_0205e84c(s32);
extern "C" M2C_UNK func_020bc190(s32);
extern "C" void *func_020b95bc(s32, void *);

enum {
    STATE_START  = 0x0,
    STATE_HOLD   = 0x1,
    STATE_FINISH = 0x5,
    HOLD_MASK    = 0x2,     // Buttons bit that keeps STATE_HOLD running.
    FINISH_MASK  = 0xf0,  // Buttons bits that choose notify id 0xb over 9.
    COUNT_LIMIT  = 0x41,  // STATE_HOLD stops once count reaches this.
    DONE_FLAG    = 0x1,     // Task::flags bit set when STATE_FINISH completes.
    SLOT_COUNT   = 0x2,
};

static inline Anchor *owner_item(Owner *o, s32 n) {
    return o->items[n];
}

// Releases a slot object through func_020bc190 then func_0205e84c.
static inline void release_item(s32 item) {
    if (item != 0) {
        func_020bc190(item);
        func_0205e84c(item);
    }
}

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

// Per-frame update of a three-state task (state byte at +0xd).
// STATE_START: stores the func_020b95bc(0x4d, owner) result in handle, sends
//   notify(0x1c, 1, 1, 0), records owner->now as start, sets the owner timer
//   to 0.4 (fixed point) and clears owner->flag40.
// STATE_HOLD: while HOLD_MASK is held and count < COUNT_LIMIT the owner timer
//   is 0.4, or 0.115 once now - start reaches 1.2; otherwise it is 0. A timer
//   <= 0 moves to STATE_FINISH and sends notify(4, 0, 1, 0). count counts calls.
// STATE_FINISH: waits for owner->flag40, then sends notify(0xb or 9, 0, 1, 0),
//   runs the slot release pass and sets DONE_FLAG without the position copy.
// Every other exit copies owner->items[i + 1]->pos into slots[i]->pos for each
// non-null slot whose position differs, marking it dirty.
extern "C" void func_02047804(Task *self) {
    Owner *owner = self->owner;
    Owner *finish_owner;
    s32 start_timer, hold_limit, fast_timer, slow_timer;
    u32 i;

    switch (self->state) {
        case STATE_START:
            self->handle = func_020b95bc(0x4d, owner);
            self->owner->notify(0x1c, 1, 1, 0);
            self->state = STATE_HOLD;
            self->count = 0;
            self->start = self->owner->now;
            func_02007f24(&start_timer, 0.4);
            self->owner->timer  = start_timer;
            self->owner->flag40 = 0;
            self->arg10         = 4;
            break;
        case STATE_HOLD:
            if ((owner->buttons & HOLD_MASK) && (u32) self->count < COUNT_LIMIT) {
                func_02007f24(&hold_limit, 1.2);
                if (self->owner->now - self->start >= hold_limit) {
                    func_02007f24(&fast_timer, 0.115);
                    self->owner->timer = fast_timer;
                } else {
                    func_02007f24(&slow_timer, 0.4);
                    self->owner->timer = slow_timer;
                }
            } else {
                self->owner->timer = 0;
            }
            if (self->owner->timer <= 0) {
                self->state = STATE_FINISH;
                self->owner->notify(4, 0, 1, 0);
            }
            func_02047b94(self, 1);
            func_02047b08(self);
            self->count++;
            break;
        case STATE_FINISH:
            func_02047b94(self, 1);
            finish_owner = self->owner;
            if (finish_owner->flag40 != 0) {
                if (finish_owner->buttons & FINISH_MASK) {
                    finish_owner->notify(0xb, 0, 1, 0);
                } else {
                    finish_owner->notify(9, 0, 1, 0);
                }
                for (i = 0; i < SLOT_COUNT; i++) {
                // The binary tests for an empty slot here, so the release
                // never runs and only the zero store remains.
                    if (self->slots[i] == 0) {
                        release_item((s32) self->slots[i]);
                        self->slots[i] = 0;
                    }
                }
                self->flags |= DONE_FLAG;
                return;
            }
            func_02047b08(self);
            break;
    }
    for (u32 j = 0; j < SLOT_COUNT; j++) {
        if (self->slots[j] != 0) {
            Item *item     = self->slots[j];
            Anchor *anchor = owner_item(self->owner, j + 1);
            if (vec_ne(&item->pos, &anchor->pos)) {
                vec_set(&item->pos, &anchor->pos);
                item->dirty = 1;
            }
        }
    }
}
