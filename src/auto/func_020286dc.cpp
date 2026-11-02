/* func_020286dc: switch the owner of a link to a new type value.
 *
 * self->f0 points at the OwnerLink base embedded at +4 of an Owner, so the
 * static_cast back to Owner subtracts 4 (NULL stays NULL).
 * The new value is 0x11 when flag is set, else self->fc; the function returns
 * early when owner->f180 already holds it. The word behind owner->f148
 * (0 if that pointer is NULL) is read before the change and handed to
 * func_0209f948 at the end. Only when flag is clear the owner's fd8 block is
 * refreshed from func_0201ca1c(f180) and func_0201c778/func_02006220.
 * The three virtual calls go through the Owner vtable slots 0x30, 0x34, 0x24.
 */
#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

// Secondary base of Owner, embedded at +4; links point at it.
class OwnerLink {
public:
    u32 f0;
};

class OwnerBase {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24(s32 a, s32 b, s32 c, s32 d); // Slot 0x24.
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30(s32 a, s32 b); // Slot 0x30.
    virtual void unknown34(s32 a); // Slot 0x34.
};

class Owner : public OwnerBase, public OwnerLink {
public:
    u8 pad0[0xd4 - 0x8];
    u8 fd4[0x4];
    u8 fd8[0x8];
    void *fe0;
    u8 pad1[0x148 - 0xe4];
    UnkVal *f148;
    u8 pad2[0x180 - 0x14c];
    u16 f180;
};

struct Ref {
    OwnerLink *f0;
    u8 pad0[0xc - 0x4];
    s32 fc;
};

extern "C" void func_02006220(Owner *arg0, void *arg1);
extern "C" void func_0201c778(u16 *values);
extern "C" u16 func_0201ca1c(u16 arg0);
extern "C" void func_02064fa0(void *arg0);
extern "C" void func_0209f948(void *o, UnkVal val);
extern "C" s32 func_020ab40c(void *arg0, u16 arg1);
extern "C" s32 func_020ab574(void *arg0, void *arg1, s32 arg2);

static inline Owner *ref_owner(Ref *r) {
    return static_cast<Owner *>(r->f0);
}

extern "C" void func_020286dc(Ref *self, s32 flag) {
    UnkVal saved;
    UnkVal *src;
    u16 values[6];
    u16 type;

    type = ref_owner(self)->f180;
    if (type == 0x11 && flag != 0) {
        return;
    }
    if (type == self->fc && flag == 0) {
        return;
    }
    src = ref_owner(self)->f148;
    if (src != 0) {
        saved = *src;
    } else {
        saved.v = 0;
    }
    if (flag != 0 && ref_owner(self) != 0 && ref_owner(self)->fe0 != 0) {
        func_02064fa0(ref_owner(self)->fe0);
    }
    ref_owner(self)->unknown30(flag != 0 ? 0x11 : self->fc, 0);
    if (flag == 0) {
        OwnerLink *link = self->f0;
        u16 t           = static_cast<Owner *>(link)->f180;
        func_020ab40c(static_cast<Owner *>(link)->fd8, func_0201ca1c(t));
        func_0201c778(values);
        func_02006220(ref_owner(self), values);
        func_020ab574(ref_owner(self)->fd8, values, 1);
    }
    ref_owner(self)->unknown34(1);
    ref_owner(self)->unknown24(0x19, 0, 1, 0);
    func_0209f948(ref_owner(self)->fd4, saved);
}
