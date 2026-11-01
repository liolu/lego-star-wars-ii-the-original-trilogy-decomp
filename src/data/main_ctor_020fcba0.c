/* compiler: 2.0/sp1p5 */
#include "m2c.h"

extern u8 __sinit_020e4c60[];
#pragma define_section TABLE ".ctor"    \
                             ".ctor" RW
__declspec(TABLE) u32 ARM9_CTOR_START[1] = {(u32) &__sinit_020e4c60};
