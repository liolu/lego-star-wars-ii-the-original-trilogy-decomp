#include "m2c.h"

typedef struct {
    u16 lo;
    u16 kind;
    u16 unk;
} Range;

typedef struct {
    s32 v;
} Fx;

s32 func_02037054(void *arg0) {
    s32 v;
    Range *r;
    void *obj;
    Fx *src;
    Fx *p;
    Fx tmp;

    obj = M2C_FIELD(arg0, void **, 4);
    src = M2C_FIELD(obj, Fx **, 0x148);
    r   = M2C_FIELD(obj, Range **, 0x174);
    if (src != 0) {
        tmp = *src;
        p   = &tmp;
    } else {
        tmp.v = 0;
        p     = &tmp;
    }
    v = p->v >> 12;
    if (r != 0) {
        while (v >= r->lo) {
            if (r->kind == 6 || r->kind == 2) {
                return 1;
            }
            r++;
        }
    }
    return 0;
}
