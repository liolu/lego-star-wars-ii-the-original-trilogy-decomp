/* compiler: 2.0/sp1p5 -thumb */
typedef unsigned int u32;
typedef int s32;

typedef struct {
    u32 a;
    u32 b;
    u32 c;
    u32 d;
    u32 e;
    u32 f;
    u32 g;
    u32 h;
    u32 i;
} S36;

void func_020c8228(S36 *p, s32 x, s32 y) {
    p->a = 0x1000;
    p->b = 0;
    p->c = 0;
    p->d = 0;
    p->e = y;
    p->f = x;
    p->g = 0;
    p->h = -x;
    p->i = y;
}
