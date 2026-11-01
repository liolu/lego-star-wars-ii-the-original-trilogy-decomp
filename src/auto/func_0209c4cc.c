/* func_0209c4cc: sets up a text block at arg0 + 0x34 and a child object at
 * arg0 + 0xb4, then prints two multi-line strings into the text block.
 *
 * Steps:
 *  1. Configure the text block (func_02065600 mode 4, func_0206574c style 6),
 *     print string 0x12b (looked up by func_02066708) and switch to style 0.
 *     func_02065be8 returns the running text position used by later prints.
 *  2. Ask func_0209d840 for two string ids (selector 0 and 1) from the table
 *     reached through *data_02107860 (+0x10), keyed by byte 0x2a of
 *     data_021027f8; -1 means "no string".
 *  3. Create the child object through func_0201ad50 (two 0x8000 by-value
 *     words, as in the other callers), set bit 3 of its word at +0x8 and pass
 *     0x36a to func_0201b2b8.
 *  4. For each id that is not -1, print the string one field (line) at a time
 *     with func_02065af8, which advances the string pointer and decrements the
 *     remaining-width counter; line n of the first string starts at
 *     0x2a + n * 0x15, of the second at 0x64 + n * 0x15.
 */
#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u8 pad[0x10];
    s32 id;
} Holder;

typedef struct {
    u8 pad[8];
    u32 flags;
} Obj;

void func_0201ad50(void *, s32, void *, UnkVal, UnkVal, u8, s32);
void func_0201b2b8(void *, s16, s32);
void func_02065600(void *, s32);
void func_0206574c(void *, s32, s32);
s32 func_02065af8(void *, s32, s32, s32, s32, u8 **, s32, s32 *);
s32 func_02065be8(void *, s32, s32, s32, void *, s32);
u8 *func_02066708(s32);
s16 func_0209d840(s32, u8, u32);

extern Holder *data_02107860;
extern u8 data_021027f8[];
extern u8 data_020f7728[];
extern s32 data_027e02a4;

enum {
    TEXT_OFF        = 0x34,
    CHILD_OFF       = 0xb4,
    TITLE_STR_ID    = 0x12b,
    NO_STRING       = -1,
    FIRST_LINE_0    = 0x2a,
    FIRST_LINE_1    = 0x64,
    LINE_STEP       = 0x15,
    WIDTH_UNLIMITED = 0x7FFFFFFF,
};

void func_0209c4cc(void *arg0) {
    s32 pos;
    u8 *text0;
    s32 width0;
    u8 *text1;
    s32 width1;
    UnkVal y;
    UnkVal x;
    u8 line;
    s32 str_id0;
    s32 str_id1;
    void **mgr; /* address of data_02107860, see FAKE below */

    func_02065600(arg0 + TEXT_OFF, 4);
    func_0206574c(arg0 + TEXT_OFF, 6, -1);
    pos = func_02065be8(arg0 + TEXT_OFF, 1, 0x80, 8, func_02066708(TITLE_STR_ID), 1);
    func_0206574c(arg0 + TEXT_OFF, 0, -1);
    mgr = (void **) &data_02107860; // FAKE: cached, type-punned address of the manager pointer (keeps its literal in a
                                    // callee-saved register across the first func_0209d840 call)
    str_id0 = func_0209d840(((Holder *) *mgr)->id, data_021027f8[0x2a], 0);
    str_id1 = func_0209d840(((Holder *) *mgr)->id, data_021027f8[0x2a], 1);
    x.v     = 0x8000;
    y.v     = 0x8000;
    func_0201ad50(arg0 + CHILD_OFF, 1, data_020f7728, x, y, 0, data_027e02a4);
    ((Obj *) *(void **) (arg0 + CHILD_OFF))->flags |= 8;
    func_0201b2b8(*(void **) (arg0 + CHILD_OFF), 0x36a, 0);
    if (str_id0 != NO_STRING) {
        text0  = func_02066708(str_id0);
        width0 = WIDTH_UNLIMITED;
        line   = 0;
        while (*text0 != 0) {
            pos  = func_02065af8(arg0 + TEXT_OFF, pos, 0x80, line * LINE_STEP + FIRST_LINE_0, 0xe8, &text0, 1, &width0);
            line = line + 1;
        }
    }
    if (str_id1 != NO_STRING) {
        text1  = func_02066708(str_id1);
        width1 = WIDTH_UNLIMITED;
        line   = 0;
        while (*text1 != 0) {
            pos  = func_02065af8(arg0 + TEXT_OFF, pos, 0x80, line * LINE_STEP + FIRST_LINE_1, 0xe8, &text1, 1, &width1);
            line = line + 1;
        }
    }
}
