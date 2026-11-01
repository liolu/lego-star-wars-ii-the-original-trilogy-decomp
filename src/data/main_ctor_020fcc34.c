/* compiler: 2.0/sp1p5 */
#include "m2c.h"

extern u8 __sinit_020e595c[];
#pragma define_section TABLE ".ctor"    \
                             ".ctor" RW
__declspec(TABLE) u32 data_020fcc34[1] = {(u32) &__sinit_020e595c};
