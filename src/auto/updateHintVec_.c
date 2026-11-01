#include "m2c.h"

typedef struct NNSG3dAnmObj {
    u8 pad0[0x10];
    struct NNSG3dAnmObj *next;
    u8 pad14[0x5];
    u8 numMapData;
    u16 mapData[1];
} NNSG3dAnmObj;

enum {
    MAPDATA_FLAG  = 0x100, /* bit tested in each 16-bit map entry */
    BITS_PER_WORD = 32,
};

/*
 * Walks the animation-object chain (next at +0x10). For every map entry i
 * (count = numMapData) whose MAPDATA_FLAG bit is set, sets bit i of the
 * caller's word-array bitset pVec. Existing bits are kept (OR only).
 */
void updateHintVec_(u32 *pVec, const NNSG3dAnmObj *pAnmObj) {
    while (pAnmObj) {
        int i;
        for (i = 0; i < pAnmObj->numMapData; ++i) {
            if (pAnmObj->mapData[i] & MAPDATA_FLAG) {
                pVec[i >> 5] |= 1 << (i & (BITS_PER_WORD - 1));
            }
        }
        pAnmObj = pAnmObj->next;
    }
}
