/* Ce que gcc et mwcc appellent implicitement (pas de libgcc en 32 bits sur ce PC). */
#include "nitro.h"

void __main(void) {}

void *__memcpy(void *d, const void *s, u32 n) {
    u8 *a = d;
    const u8 *b = s;
    while (n--) *a++ = *b++;
    return d;
}

void __builtin__clear(void *d, u32 n) {
    u8 *a = d;
    while (n--) *a++ = 0;
}

u64 __udivdi3(u64 a, u64 b);
u64 __umoddi3(u64 a, u64 b);

/* divisions du compilateur ARM : retour r0 = quotient, r1 = reste */
u64 _u32_div_f(u32 a, u32 b) {
    if (!b) return (u64)a << 32;
    return ((u64)(a % b) << 32) | (a / b);
}
u64 _u32_div_not_0_f(u32 a, u32 b) { return _u32_div_f(a, b); }
u64 _s32_div_f(s32 a, s32 b) {
    if (!b) return (u64)(u32)a << 32;
    if (a == (s32)0x80000000 && b == -1) return (u32)a;
    return ((u64)(u32)(a % b) << 32) | (u32)(a / b);
}

u64 __udivmoddi4(u64 a, u64 b, u64 *r) {
    u64 q = __udivdi3(a, b);
    if (r) *r = a - q * b;
    return q;
}

/* Aides du compilateur ARM : certaines se passent des valeurs hors des registres d'arguments.
 * Ici elles sont ecrites directement, en double et float IEEE comme sur le DS. */
typedef union { double d; u64 u; struct { u32 lo, hi; } w; } D64;
typedef union { float f; u32 u; } F32;

#define D2(name, op) \
    u64 name(u32 a0, u32 a1, u32 b0, u32 b1) { \
        D64 a, b, r; a.w.lo = a0; a.w.hi = a1; b.w.lo = b0; b.w.hi = b1; \
        r.d = a.d op b.d; return r.u; \
    }
D2(_dadd, +)
D2(_dsub, -)
D2(_dmul, *)
D2(_ddiv, /)

#define F2(name, op) \
    u64 name(u32 a0, u32 b0) { \
        F32 a, b, r; a.u = a0; b.u = b0; r.f = a.f op b.f; return r.u; \
    }
F2(_fadd, +)
F2(_fsub, -)
F2(_fmul, *)
F2(_fdiv, /)

#define WORDS(a, b) (((u64)(b) << 32) | (a))
u64 __umoddi3(u64 a, u64 b);
s64 __divdi3(s64 a, s64 b);
s64 __moddi3(s64 a, s64 b);
u64 _ll_udiv(u32 a0, u32 a1, u32 b0, u32 b1) { u64 d = WORDS(b0, b1); return d ? __udivdi3(WORDS(a0, a1), d) : 0; }
u64 _ull_mod(u32 a0, u32 a1, u32 b0, u32 b1) { u64 d = WORDS(b0, b1); return d ? __umoddi3(WORDS(a0, a1), d) : 0; }
u64 _ll_sdiv(u32 a0, u32 a1, u32 b0, u32 b1) { s64 d = (s64)WORDS(b0, b1); return d ? (u64)__divdi3((s64)WORDS(a0, a1), d) : 0; }
u64 _ll_mod(u32 a0, u32 a1, u32 b0, u32 b1) { s64 d = (s64)WORDS(b0, b1); return d ? (u64)__moddi3((s64)WORDS(a0, a1), d) : 0; }
