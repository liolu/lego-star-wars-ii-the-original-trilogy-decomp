#include "m2c.h"

// 0x20-byte block of 16 halfwords; the last halfword holds a checksum
// chosen so that the halfword sum of the whole block is zero.
struct Header {
    u16 w[15];
    s16 check;
};

enum {
    HEADER_SIZE = 0x20,
};

extern "C" u8 data_020edc58[];

extern "C" s32 func_0205e6e0(void *dst, const void *src, s32 size);
extern "C" u16 func_02063510(void *buf, u32 size); // Sum of the halfwords in buf.
extern "C" s32 func_02063648(s32 arg0, s32 arg1, void *buf);

// FAKE: second (namespace-scope) declaration of the same extern "C" object; mwcc keeps it as a separate object, giving the
// checksum store its own pool word (original likely used a second name for this address).
namespace save {
    extern "C" Header data_02104364;
}
extern "C" Header data_02104364;

// Fills the header block from data_020edc58, seals it with the negated
// halfword sum, then hands it to func_02063648 (args 0, 2) and returns its result.
extern "C" s32 func_0206353c(void) {
    func_0205e6e0(&data_02104364, data_020edc58, HEADER_SIZE);
    save::data_02104364.check = -func_02063510(&data_02104364, HEADER_SIZE);
    return func_02063648(0, 2, &data_02104364);
}
