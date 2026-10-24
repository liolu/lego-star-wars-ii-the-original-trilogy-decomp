/* Reconstructed from USA ARM9 at 0x0209eba0 (104 bytes).
 *
 * Virtual slot 0x08 of the class whose vtable is data_020ffb64 (see
 * func_0209eadc / func_0209ea20). It reads a halfword from the graphics
 * state at +0x0c (func_02053adc), adds the angle 0xb6 (one degree in
 * 16 bit angle units), passes the sum by value to func_02053850 (a two
 * byte structure goes through a stack slot below sp) and then runs the two
 * follow-up routines on the object.
 *
 * The two halfword scratch slots (angle and a dead copy) are kept in memory:
 * the dead copy by taking its address, the angle by a volatile access. The
 * result is read through a pointer to the local that receives the value.
 */
#include "m2c.h"

typedef struct {
    u16 v;
} Half;

extern void func_02053adc(u16 *, void *);
extern void func_02053850(void *, Half);
extern void func_0209ec08(void *);
extern void func_0209ecc0(void *);

void func_0209eba0(void *self) {
    Half res;
    u16 out;
    u16 spare;
    volatile u16 angle;
    void *gfx = M2C_FIELD(self, void **, 0xC);
    u16 *pout;
    u16 *pspare;

    pspare = &spare;
    spare  = 0xB6;
    angle  = 0xB6;
    func_02053adc(&out, gfx);
    pout = &out;
    if (pout) {
        res.v = *pout + angle;
    } else {
        res.v = *pspare;
    }
    func_02053850(M2C_FIELD(self, void **, 0xC), res);
    func_0209ec08(self);
    func_0209ecc0(self);
}
