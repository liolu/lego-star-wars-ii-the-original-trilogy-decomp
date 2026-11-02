#include "m2c.h"

typedef struct {
    u32 sig;
    u8 pad[8];
} Entry;

typedef struct {
    u8 pad[0x1e];
    u16 sum;
} Header;

extern Header data_02104364;
extern Entry data_02104388[];

u16 func_02063510(u16 *arg0, u32 arg1);
s32 func_02063610(s32 arg0, s32 arg1, void *arg2);
s32 func_02063648(s32 arg0, s32 arg1, void *arg2);

// FAKE: reach the header from the slot array so the data base is a separate literal.
static inline Header *header_base(void) {
    return (Header *) (data_02104388 - 3);
}

enum {
    HEADER_FIRST_BLOCK = 0, /* block index; the callees scale by 0x10 bytes */
    HEADER_BLOCKS      = 2, /* 2 x 0x10 bytes = sizeof(Header) */
    HEADER_WORDS       = 8,
};

/* Fetches the 0x20-byte header copy via func_02063610 (0 = failure, returned as 0).
 * Otherwise re-seals the in-memory header: clears sum, then stores the negated 16-bit
 * halfword sum so the whole header sums to zero. If the sealed header differs from the
 * fetched copy in any word, passes it to func_02063648 and returns that result;
 * else returns the fetch result. */
s32 func_02063468(void) {
    u32 fetched[HEADER_WORDS];
    s32 result;
    u32 *cur;
    u32 *ref;
    u32 words_left;

    result = func_02063610(HEADER_FIRST_BLOCK, HEADER_BLOCKS, fetched);
    if (result == 0) {
        return 0;
    }
    data_02104364.sum = 0;
    data_02104364.sum = -func_02063510((u16 *) header_base(), sizeof(Header));
    cur               = (u32 *) header_base();
    ref               = fetched;
    words_left        = HEADER_WORDS;
    do {
        if (*cur != *ref) {
            break;
        }
        cur++;
        ref++;
    } while (--words_left != 0);
    if (words_left != 0) {
        result = func_02063648(HEADER_FIRST_BLOCK, HEADER_BLOCKS, header_base());
    }
    return result;
}
