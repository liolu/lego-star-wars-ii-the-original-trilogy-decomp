#include "m2c.h"

/* func_020bf198 - reads a fixed direction record and stores it as a vector.
 *
 * func_020be664 consumes the common record header from `in` and returns the
 * record body: a length word, a yaw and a pitch halfword, then one more word.
 * The body is turned into a spherical spec with the length word in both
 * length-source slots and zero yaw/pitch spread, so func_020bd75c yields a
 * fixed-point vector for that length and direction.  The vector is copied into self->v (+0x14), the last
 * body word into self->w (+0x20), and the pointer past the 12-byte body is
 * returned.  Reached through the pointer table at data_02101204.
 *
 * The vector goes through a transform-like temporary whose rotation half is
 * zeroed but never read; those dead stores stay because the temporary's
 * address is compared.
 */

typedef struct {
    s32 x, y, z;
} V3;

typedef struct {
    s16 x, y, z, w;
} Ang;

typedef struct {
    s32 a;
    s32 b;
    u16 yaw;
    u16 yawSpread;
    u16 pitch;
    u16 pitchSpread;
} Spec;

typedef struct {
    Ang rot;
    V3 pos;
} Xform;

typedef struct {
    u8 pad[0x14];
    V3 v;
    s32 w;
} Obj;

extern u32 *func_020be664(Obj *, u32 *);
extern void func_020bd75c(V3 *, Spec *);

u32 *func_020bf198(Obj *self, u32 *in) {
    u32 *body = func_020be664(self, in);
    Spec spec;
    V3 vec;
    Xform xf;
    u16 pitch;
    spec.a           = 0;
    spec.b           = 0;
    spec.yaw         = 0;
    spec.yawSpread   = 0;
    spec.pitch       = 0;
    spec.pitchSpread = 0;
    spec.a           = body[0];
    spec.yaw         = ((u16 *) body)[2];
    xf.rot.y         = 0; // FAKE: dead store placed between the yaw and pitch reads to steer scheduling
    pitch          = ((u16 *) body)[3]; // FAKE: pitch read into a local so the next two dead stores sit between load and store
    xf.rot.w       = 0; // FAKE: dead store placed to steer scheduling
    xf.rot.z       = 0; // FAKE: dead store placed to steer scheduling
    spec.pitch     = pitch;
    spec.b         = spec.a;
    spec.yawSpread = 0;
    xf.rot.x       = 0; // FAKE: dead store placed between the spread stores to steer scheduling
    spec.pitchSpread = 0;
    func_020bd75c(&vec, &spec);
    xf.pos = vec;
    if (&self->v != &xf.pos) {
        self->v.x = xf.pos.x;
        self->v.y = xf.pos.y;
        self->v.z = xf.pos.z;
    }
    self->w = body[2];
    return body + 3;
}
