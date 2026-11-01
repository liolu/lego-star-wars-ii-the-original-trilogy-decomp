#include "m2c.h"

typedef struct ItemDef {
    u16 kind;
    u16 pad02;
    u16 model;
    u16 sound;
    u8 slot;
    u8 part;
    u8 pad0a[0x19 - 0xa];
    u8 flag19;
    u8 pad1a[0x27 - 0x1a];
    u8 mode;
    u8 flag28;
} ItemDef;

// Part objects dispatch through vtable slot 0x08 (same shape as func_0200636c).
// Declarations only: the original vtable remains in the ROM data.
class Obj {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08(u32, u32, u32, u32); // Slot 0x08.
};

typedef struct Part {
    Obj *obj;
    u8 pad04[0xe - 0x4];
    u8 fe;
    u8 pad0f;
    u8 f10;
    u8 pad11[0x28 - 0x11];
} Part;

typedef struct Actor {
    u8 pad000[0xe0];
    u8 *fe0;
    u8 pad0e4[0x180 - 0xe4];
    u16 f180;
    u8 pad182[0x1d0 - 0x182];
    Part parts[3];
    u8 *f248;
    u8 pad24c[0x6bb - 0x24c];
    u8 f6bb;
    u8 pad6bc[0x6e8 - 0x6bc];
    u16 slots[5];
    u8 pad6f2;
    u8 f6f3;
} Actor;

extern "C" u8 data_02104d90[];

extern "C" s32 func_020867ec(void *arg0, s32 arg1);
extern "C" ItemDef *func_0208dad8(u16 id);
extern "C" void func_02006ab4(Actor *self, s32 sound, s32 slot, void *arg3);
extern "C" void func_02003b5c(Actor *self);
extern "C" void func_02006360(Actor *self);
extern "C" void func_0200c724(Actor *self, s32 model, u8 part, s8 v);
extern "C" u16 func_0200c8c4(Actor *self, s32 part);
extern "C" void func_0200c7dc(Actor *self, s32 part);
extern "C" s32 func_02010c54(s32 index, s32 value);
extern "C" void func_0201c778(u16 *values);
extern "C" void func_0201c858(u16 *values, s32 index, u16 value);
extern "C" s32 func_020ab574(void *arg0, u16 *values, s32 arg2);

enum {
    SLOT_EMPTY = 0xfa,  // slots[] value meaning "nothing stored"; never passed to func_0208dad8.
    ID_NONE    = 0xffff,   // ItemDef halfword meaning "no id"; skips the dependent call.
};

// Stores item `id` (remapped 0x64/0xa9/0xe1 -> next id when func_020867ec(data_02104d90, 0xf)
// is set) in self->slots[def->slot], runs the per-slot handling (func_02006ab4 with the slot
// number, func_02006360 / func_0200c7dc refreshes), then for a changed part model calls
// func_0200c724, the part object's slot-0x08 virtual and the func_0201c778/func_0201c858/
// func_020ab574 sequence on a 5-halfword buffer. Always ends with func_02003b5c(self).

extern "C" void func_020063d4(Actor *self, u16 id, u8 *arg2) {
    ItemDef *def;
    ItemDef *prev;
    u16 sound;
    u16 values[5];

    if (func_020867ec(data_02104d90, 0xf) != 0) {
        if (id == 0x64) {
            id = 0x65;
        } else if (id == 0xa9) {
            id = 0xaa;
        } else if (id == 0xe1) {
            id = 0xe2;
        }
    }
    if (arg2 == 0) {
        arg2 = self->fe0 + 0x14;
    }
    def  = func_0208dad8(id);
    prev = 0;
    if (self->slots[0] != SLOT_EMPTY) {
        prev = func_0208dad8(self->slots[0]);
    }
    self->slots[def->slot] = id;

    switch (def->slot) {
        case 0:
            if (def->sound != ID_NONE) {
                func_02006ab4(self, def->sound, 0, arg2);
            }
            if (def->model == 0x276 || (self->slots[3] != SLOT_EMPTY && func_0208dad8(self->slots[3])->flag28 != 0)) {
                func_02003b5c(self);
                return;
            }
            func_02006360(self);
            break;
        case 1:
            if (def->sound != ID_NONE) {
                func_02006ab4(self, def->sound, 1, arg2);
            }
            if (def->kind == 0xb) {
                if (self->slots[0] != SLOT_EMPTY && func_0208dad8(self->slots[0])->model != 0x276) {
                    func_02003b5c(self);
                    return;
                }
                func_02006360(self);
                break;
            }
            func_02003b5c(self);
            return;
        case 2:
            if (def->sound != ID_NONE) {
                func_02006ab4(self, def->sound, 2, arg2);
            }
            func_02003b5c(self);
            return;
        case 3:
            if (self->slots[0] == SLOT_EMPTY || func_0208dad8(self->slots[0])->model == 0x276) {
                self->f6f3 = 0;
            }
            if ((def->flag28 != 0 && self->slots[0] != SLOT_EMPTY && func_0208dad8(self->slots[0])->kind != 5) ||
                (id == 0xb1 && !(self->slots[0] != SLOT_EMPTY &&
                                 (func_0208dad8(self->slots[0])->kind == 5 || func_0208dad8(self->slots[0])->kind == 0xc)))) {
                if (self->slots[0] != SLOT_EMPTY) {
                    if (func_0208dad8(self->slots[0])->model != 0x276) {
                        if (id == 0xb1) {
                            func_02006360(self);
                            func_02003b5c(self);
                            return;
                        }
                    } else {
                        func_02006360(self);
                    }
                } else {
                    func_02006360(self);
                }
            } else if (def->flag28 != 0) {
                if (self->f248 != 0) {
                    func_0200c7dc(self, 3);
                }
                func_02006360(self);
            }
            if (self->slots[0] != SLOT_EMPTY &&
                (func_0208dad8(self->slots[0])->kind == 5 || func_0208dad8(self->slots[0])->kind == 0xc) && id == 0xb1) {
                func_02003b5c(self);
                return;
            }
            break;
        case 4: {
            s32 cur = self->slots[4];
            if (cur != SLOT_EMPTY) {
                ItemDef *other = func_0208dad8(cur);
                if (self->parts[other->part].obj != 0) {
                    func_0200c7dc(self, other->part);
                }
            }
            break;
        }
    }

    if ((def->model != 0x276 && def->model != func_0200c8c4(self, def->part)) || (def->kind == 0xb && def->slot == 1)) {
        if ((def->kind == 0xb && def->slot == 1)) {
            func_0200c724(self, 0x1c1, 3, 3);
        } else {
            if (((u16) (id + 0xff3e) <= 1 && self->f6bb >= 0x49 && self->f6bb <= 0x4b &&
                 (prev == 0 || prev->model == 0x276))) {
                func_0200c724(self, 0x1c1, 3, 3);
                func_02006360(self);
            }
            func_0200c724(self, def->model, def->part, def->part);
            if (def->flag19 != 0) {
                self->parts[def->part].f10 = 1;
            }
        }
        if (def->slot == 3 && self->f180 == 5) {
            s32 res = 0;
            if (def->mode == 2 || def->mode == 0) {
                res = func_02010c54(def->model, 0x64);
            } else if (def->mode == 1) {
                res = func_02010c54(def->model, 0x65);
            }
            if (res != 0) {
                self->parts[def->part].obj->unknown08(res, 0, 1, 0);
                self->parts[def->part].fe = 1;
            }
        }
        sound = def->sound;
        if ((def->kind == 0xb && def->slot == 1) || id == 0xb1 ||
            ((u16) (id + 0xff3e) <= 1 && self->f6bb >= 0x49 && self->f6bb <= 0x4b && (prev == 0 || prev->model == 0x276))) {
            if (self->slots[0] == SLOT_EMPTY) {
                sound = ID_NONE;
            } else {
                sound = func_0208dad8(self->slots[0])->sound;
            }
        }
        if (sound != ID_NONE) {
            func_0201c778(values);
            func_0201c858(values, 0, sound);
            if (def->slot == 4 && (id == 0xe8 || id == 0xef)) {
                func_0201c858(values, 1, 0x269);
            }
            if ((def->kind == 0xb && def->slot == 1) || ((u16) (id + 0xff3e) <= 1 && self->f6bb >= 0x49 &&
                                                         self->f6bb <= 0x4b && (prev == 0 || prev->model == 0x276))) {
                func_020ab574(self->f248 + 4, values, 1);
            } else {
                func_020ab574((u8 *) self->parts[def->part].obj + 4, values, 1);
            }
        }
    }
    func_02003b5c(self);
}
