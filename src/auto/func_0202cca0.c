#include "m2c.h"

typedef struct {
    s32 thr0;
    s32 thr1;
    s32 thr2;
    u8 pad;
    u8 next;
    u8 pad2[2];
    const u8 *choices;
} Row;

extern u8 data_020ecea0[];
extern u8 data_02104d90[];
typedef struct {
    u8 type;
    u8 pad[0x23];
} Ent;
extern Ent data_020f6bab[];
extern Row data_020fdc60[];

extern s32 func_020835fc(void *, void *);
extern s32 func_020867ec(void *, s32);
extern void *func_0202cb44(void *);
extern u8 func_0202cb7c(void *);
extern u8 func_0202cc34(void *, const void *);
extern void func_02007eb4(s32 *, void *, void *);

typedef struct {
    u8 pad0[8];
    u32 flags;
} Base;

/* FAKE: rd() is a pass-through load helper; reading through it keeps the threshold load order of the target */
static inline s32 rd(const s32 *p) {
    return *p;
}

/*
 * Picks a code for object->+4 (obj). func_0202cc80 stores the result in
 * object->+9 and passes it to func_02004b78.
 * Returns 0 if obj->+8 has any of bits 0x802 set. Returns 0x23 if obj->+0x6b0
 * is set and func_020835fc(obj->+0x6b0, obj + 0xb0) returns 0.
 * Otherwise:
 * - state = data_020ecea0[func_0202cb7c(obj + 0x5e0)].
 * - kind (the row of data_020fdc60, 20-byte rows) starts as object->+8.
 * - kind becomes 4 when func_020867ec(data_02104d90, 9) is nonzero and the
 *   data_020f6bab entry for obj->+0x6c3 has type 0, 2 or 3.
 * - kind becomes row[kind].next when s16 obj->+0x6b6 < (s16 obj->+0x6b8 >> 1).
 * For state 9 the entry from func_0202cb44(obj + 0x5e0) decides:
 * - none: state 6.
 * - entry +0xc in 1..2 and entry +0x182 == 0x71: returns 0x20.
 * - otherwise func_02007eb4 gives d from obj + 0xb0 and entry + 0xb0. State is
 *   5 if func_020835fc rejects the entry. If not, row thr0/thr1/thr2 bands pick
 *   it: below thr0 gives 4 or 3 (by bit 2 of obj->+0x1a8), below thr1 gives 2,
 *   below thr2 gives 1, else 0.
 * Always clears bit 2 of obj->+0x1a8. Unless returning 0x20, it returns
 * func_0202cc34(object, row[kind].choices + state * 10).
 */
u32 func_0202cca0(void *object) {
    u8 *p = M2C_FIELD(object, u8 **, 4);
    u8 state;
    u8 kind;
    u8 *e;
    u8 *q;
    u32 result;
    s32 off;
    s32 d;
    s32 dist;

    if (M2C_FIELD(p, u32 *, 8) & 0x802) {
        return 0;
    }
    if (M2C_FIELD(p, void **, 0x6b0) != NULL) {
        if (func_020835fc(M2C_FIELD(p, void **, 0x6b0), p + 0xb0) == 0) {
            return 0x23;
        }
    }
    state = data_020ecea0[func_0202cb7c(M2C_FIELD(object, u8 **, 4) + 0x5e0)];
    kind  = M2C_FIELD(object, u8 *, 8);
    if (func_020867ec(data_02104d90, 9) != 0) {
        u8 t = data_020f6bab[M2C_FIELD(M2C_FIELD(object, u8 **, 4), u8 *, 0x6c3)].type;
        if (t == 2 || t == 0 || t == 3) {
            kind = 4;
        }
    }
    p = M2C_FIELD(object, u8 **, 4);
    if (M2C_FIELD(p, s16 *, 0x6b6) < (M2C_FIELD(p, s16 *, 0x6b8) >> 1)) {
        kind = data_020fdc60[kind].next;
    }
    result = 0;
    if (state == 9) {
        e = func_0202cb44(p + 0x5e0);
        q = NULL;
        if (e != NULL && (u32) (M2C_FIELD(e, u16 *, 0xc) - 1) <= 1) {
            q = e;
        }
        if (e == NULL) {
            state = 6;
        } else if (q != NULL && M2C_FIELD(q, u16 *, 0x182) == 0x71) {
            result = 0x20;
        } else {
            func_02007eb4(&d, M2C_FIELD(object, u8 **, 4) + 0xb0, e + 0xb0);
            if (M2C_FIELD(M2C_FIELD(object, u8 **, 4), void **, 0x6b0) != NULL &&
                func_020835fc(M2C_FIELD(M2C_FIELD(object, u8 **, 4), void **, 0x6b0), e + 0xb0) == 0) {
                state = 5;
            } else {
                off  = kind * 20; /* byte offset of row kind in data_020fdc60 */
                dist = d;
                if (rd(&d) < *(s32 *) ((u8 *) &data_020fdc60[0].thr0 + off)) {
                    if (M2C_FIELD(M2C_FIELD(object, u8 **, 4), u16 *, 0x1a8) & 2) {
                        state = 4;
                    } else {
                        state = 3;
                    }
                } else if (dist < *(s32 *) ((u8 *) &data_020fdc60[0].thr1 + off)) {
                    state = 2;
                } else if (dist < *(s32 *) ((u8 *) &data_020fdc60[0].thr2 + off)) {
                    state = 1;
                } else {
                    state = 0;
                }
            }
        }
    }
    p = M2C_FIELD(object, u8 **, 4);
    M2C_FIELD(p, u16 *, 0x1a8) &= ~2;
    if (result == 0) {
        result = func_0202cc34(object, data_020fdc60[kind].choices + state * 10);
    }
    return result;
}
