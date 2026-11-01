#include "m2c.h"

/* Creates a 0x6f8-byte object at the actor-local offset (0, 1.0, 8.0) (12-bit fixed point; rotated by the
   actor matrix, rebuilt first if the rotation is dirty, then scaled and moved to world space), yaw copied from the
   actor. The create id is the actor variant, replaced (variants below 0x54 only) when query 0xe or 0x10 of
   data_02104d90 succeeds, by one of two ids per query picked by func_0207b68c. While either query succeeds, the
   variant table's halfword is passed to the new object. Then sets object flag 0x2000, copies actor word 0x218,
   applies the variant 0x40 / 0x5f extras, hands the object to func_02025024 and sets actor byte 0x231 to 90. */
enum {
    FIXED_ONE        = 0x1000,
    FIXED_EIGHT      = 0x8000,
    OBJECT_SIZE      = 0x6f8,
    QUERY_A          = 0xe,
    QUERY_B          = 0x10,
    VARIANT_LIMIT    = 0x54,
    ID_A_SET         = 0x43,
    ID_A_CLEAR       = 0x10,
    ID_B_SET         = 0x20,
    ID_B_CLEAR       = 9,
    OBJECT_FLAG      = 0x2000,
    VARIANT_EXTRA    = 0x40,
    EXTRA_ARG        = 0x22,
    VARIANT_PAIRED   = 0x5f,
    PART_KIND_LINKED = 3,
    COUNTER_START    = 0x5a,
};

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    s32 value;
} Fixed;
typedef struct {
    Fixed x, y, z;
} Scale;
typedef struct {
    u8 pad[0xe];
    u8 kind;
} Part;
typedef struct {
    u8 pad[0x1a8];
    u16 flags;
    u8 pad1aa[0x5f8 - 0x1aa];
    Part *part;
    u8 pad5fc[0x6b0 - 0x5fc];
    s32 value;
} Object;
typedef struct {
    u8 pad00[8];
    u32 flags;
    u8 pad0c[0x80];
    s32 matrix[9];
    Vec3 position;
    Scale scale;
    u16 rotation[4];
    u8 rotationDirty;
    u8 matrixDirty;
    u8 pad_d2[0x218 - 0xd2];
    s32 value;
    u8 variant;
    u8 pad21d[0x231 - 0x21d];
    u8 counter;
} Actor;
typedef struct {
    u8 data[0x48];
} Template;
typedef struct {
    u16 id;
    u8 pad[0x16];
} VariantInfo;

extern const VariantInfo data_020ef58a[];
extern u8 data_02104d90;
extern u8 data_0210930c;

extern void func_01ffa6f8(void *, const void *);
extern void func_01ffa958(Vec3 *, const void *, const Vec3 *);
extern void *func_0205e80c(s32, s32, s32);
extern void func_020accac(void *, void *, void *, void *);
extern void *func_020029c4(void *, s32, s32, s32, void *);
extern void func_020043f4(void *);
extern s32 func_020867ec(void *, s32);
extern s32 func_0207b68c(s32);
extern void func_02003998(void *, u8);
extern void func_02004b78(void *, s32);
extern void func_02093568(s32 *, void *, Vec3 *, s32 *);
extern void func_02035720(void *, s32);
extern void func_02025024(Actor *, void *);

static inline Vec3 *initializeVector(Vec3 *value, s32 x, s32 y, s32 z) {
    value->x = x;
    value->y = y;
    value->z = z;
    return value;
}

static inline s32 multiplyFixed(s32 value, const Fixed *scale) {
    return (s32) (((s64) value * scale->value + 0x800) >> 12);
}

static inline void worldOffset(Vec3 *result, Actor *actor, const Vec3 *offset) {
    Fixed x, y, z;
    if (actor->rotationDirty) {
        actor->matrixDirty   = 1;
        actor->rotationDirty = 0;
        func_01ffa6f8(actor->matrix, actor->rotation);
    }
    func_01ffa958(result, actor->matrix, offset);
    if (actor->scale.x.value != FIXED_ONE || actor->scale.y.value != FIXED_ONE || actor->scale.z.value != FIXED_ONE) {
        x         = actor->scale.x;
        result->x = multiplyFixed(result->x, &x);
        y         = actor->scale.y;
        result->y = multiplyFixed(result->y, &y);
        z         = actor->scale.z;
        result->z = multiplyFixed(result->z, &z);
    }
    result->x += actor->position.x;
    result->y += actor->position.y;
    result->z += actor->position.z;
}

s32 func_02024b20(Actor *actor) {
    s32 param;
    u16 rot[4];
    s32 queryOut;
    Vec3 result;
    Vec3 offset;
    s32 scl[3];
    Template tpl;
    u32 variant;
    u16 tableHalf;
    Object *obj;

    Vec3 *o = initializeVector(&offset, 0, FIXED_ONE, FIXED_EIGHT);
    worldOffset(&result, actor, o);
    variant   = actor->variant;
    tableHalf = data_020ef58a[variant].id;
    if (func_020867ec(&data_02104d90, QUERY_A) && variant < VARIANT_LIMIT) {
        if (func_0207b68c(variant)) {
            variant = ID_A_SET;
        } else {
            variant = ID_A_CLEAR;
        }
    } else if (func_020867ec(&data_02104d90, QUERY_B) && variant < VARIANT_LIMIT) {
        if (func_0207b68c(variant)) {
            variant = ID_B_SET;
        } else {
            variant = ID_B_CLEAR;
        }
    }
    obj = func_0205e80c(OBJECT_SIZE, 0, 4);
    if (obj != 0) {
        rot[0] = 0;
        rot[1] = actor->rotation[1];
        rot[2] = 0;
        scl[0] = FIXED_ONE;
        scl[1] = FIXED_ONE;
        scl[2] = FIXED_ONE;
        func_020accac(&tpl, &result, rot, scl);
        {
            s32 flag     = 1;
            Template *ps = &tpl;
            if (actor->flags & 0x80000000) {
                flag = 0;
            }
            obj = func_020029c4(obj, 2, variant, flag, ps);
        }
    }
    if (func_020867ec(&data_02104d90, QUERY_A) || func_020867ec(&data_02104d90, QUERY_B)) {
        func_02003998(obj, tableHalf);
    }
    obj->flags |= OBJECT_FLAG;
    func_020043f4(obj);
    obj->value = actor->value;
    if (actor->variant == VARIANT_EXTRA) {
        func_02004b78(obj, EXTRA_ARG);
    } else if (actor->variant == VARIANT_PAIRED) {
        Part *part = obj->part;
        if (part != 0 && part->kind == PART_KIND_LINKED) {
            param = 0;
            func_02093568(&queryOut, &data_0210930c, &result, &param);
            func_02035720(part, param);
        }
    }
    func_02025024(actor, obj);
    actor->counter = COUNTER_START;
    return 0;
}
