/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* func_02082c44 from https://github.com/cntrl-alt-lenny/gx-spirit-caller src/main/func_02082c44.legacy_sp3.c (license: MIT),
 * reused by tools/donor_match.py */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
struct State {
    char pad0[0x10];
    u32 flags;
    char pad14[0x18];
    u16 width, height;
};
typedef struct Ctx Ctx;
struct Ctx {
    u8 *pc;
    int pad4;
    u32 flags;
    char padc[0x30];
    void (*cb)(Ctx *);
    char pad40[0x58];
    u8 stage;
    char pad99[0x17];
    struct State *state;
    char padb4[0x24];
    u8 *resource;
};
extern int data_02101398[2];
extern int data_0210139c;
extern unsigned char NNS_G3dGlb[];
extern int data_027e0c60[];
extern int data_027e0cd0[];
extern void NNS_G3dGeBufferOP_N(int cmd, void *src, int n);
extern void NNS_G3dGetCurrentMtx(void *a0, void *a1);
void func_020c5738(Ctx *c) {
    int st, skip;
    int a, b, d, e, f;
    int matrix[9];
    int vec[3];
    if (!(c->flags & 0x200) && (c->flags & 1)) {
        if ((c->state->flags & 0xc0000000) != 0x80000000) {
            c->state->flags &= ~0xc0000000;
            c->state->flags |= 0x80000000;
            data_02101398[1] = c->state->flags;
            NNS_G3dGeBufferOP_N(data_02101398[0], &data_0210139c, 1);
        }
        a = 3;
        NNS_G3dGeBufferOP_N(0x10, &a, 1);
        if (c->cb != 0) {
            st = c->stage;
        } else {
            st = 0;
        }
        if (st == 1) {
            c->flags &= ~0x40;
            c->cb(c);
            if (c->cb != 0) {
                st = c->stage;
            } else {
                st = 0;
            }
            skip = c->flags & 0x40;
        } else {
            skip = 0;
        }
        if (skip == 0) {
            int width, height;
            height = c->state->height;
            width  = c->state->width;
            vec[0] = width << 15;
            vec[1] = -height << 15;
            vec[2] = 0x10000;
            NNS_G3dGeBufferOP_N(0x1b, vec, 3);
            b = (u16) (short) (width << 3) | ((u16) (short) (height << 3) << 16);
            NNS_G3dGeBufferOP_N(0x22, &b, 1);
        }
        if (st == 2) {
            c->flags &= ~0x40;
            c->cb(c);
            if (c->cb != 0) {
                st = c->stage;
            } else {
                st = 0;
            }
            skip = c->flags & 0x40;
        } else {
            skip = 0;
        }
        if (skip == 0) {
            u8 *base  = c->resource;
            u8 *table = base + 4;
            u8 *entry = table + *(u16 *) (base + 10);
            u8 *p     = base + *(u32 *) (entry + *(u16 *) entry * c->pc[1] + 4);
            int flags = *(u16 *) (p + 0x1e);
            if (flags & 0x2000) {
                p += 0x2c;
                if (!(flags & 2)) {
                    p += 8;
                }
                if (!(flags & 4)) {
                    p += 4;
                }
                if (!(flags & 8)) {
                    p += 8;
                }
                NNS_G3dGeBufferOP_N(0x18, p, 0x10);
            }
        }
        if (st == 3) {
            c->flags &= ~0x40;
            c->cb(c);
            skip = c->flags & 0x40;
        } else {
            skip = 0;
        }
        if (skip == 0) {
            d = 2;
            NNS_G3dGeBufferOP_N(0x10, &d, 1);
            NNS_G3dGetCurrentMtx(0, matrix);
            e = 3;
            NNS_G3dGeBufferOP_N(0x10, &e, 1);
            if (*(u32 *) (NNS_G3dGlb + 0xfc) & 1) {
                NNS_G3dGeBufferOP_N(0x1a, data_027e0c60, 9);
                NNS_G3dGeBufferOP_N(0x1a, data_027e0cd0, 9);
                NNS_G3dGeBufferOP_N(0x1a, matrix, 9);
            } else if (*(u32 *) (NNS_G3dGlb + 0xfc) & 2) {
                NNS_G3dGeBufferOP_N(0x1a, data_027e0c60, 9);
                NNS_G3dGeBufferOP_N(0x1a, matrix, 9);
            } else {
                NNS_G3dGeBufferOP_N(0x1a, matrix, 9);
            }
        }
        f = 2;
        NNS_G3dGeBufferOP_N(0x10, &f, 1);
    }
    c->pc += 3;
}
