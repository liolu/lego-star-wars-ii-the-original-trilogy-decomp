#include "m2c.h"

typedef struct {
    s32 v;
} Fixed;
typedef struct {
    Fixed second, first;
} Position;
typedef struct {
    u8 pad[0x90];
    void *line[4];
    void *name[4];
} Obj;
enum {
    SLOT_COUNT              = 4,
    EMPTY_LABEL             = 100,
    ROW_SPACING             = 0x28,
    OBJECT_ROW_START        = 0x20,
    VALUE_ROW_START         = 0x24,
    LABEL_RECORD_BYTES      = 0x18,
    POSITION_FRACTION_BITS  = 12,
    HEADER_FIRST_COMPONENT  = 0xd0000,
    HEADER_SECOND_COMPONENT = 0x8000,
    SLOT_FIRST_COMPONENT    = 0xc000,
    BYTE_MASK               = 0xff,
    HEADER_STRING_ID        = 0x28e,
    VALUE_LABEL_1_ID        = 0x28f,
    VALUE_LABEL_2_ID        = 0x290,
    VALUE_LABEL_3_ID        = 0x291
};

extern void func_0201ad50(void *, void *, void *, Fixed, s32, u8, void *);
extern void func_0201b2b8(void *, s16, s32);
extern void func_02055ba4(s32, s32, s32, s32, s32);
extern void func_02065600(void *, s32);
extern void func_02065658(void *, void *);
extern void func_0206574c(void *, s32, s32);
extern s32 func_02065be8(void *, s32, s32, s32, void *, s32);
extern void func_0206653c(s32, s32, s32, s32, s32, s32, s32);
extern void *func_02066708(s32);
extern s16 func_0209bc04(void *, s32);
extern void func_0209c2f0(void *, s32 *, s32, s32, s32);
extern s32 data_027e02a4;
extern u8 *data_02107860[];
extern u8 data_020ef57c[];
extern u8 data_020f7720[];
extern u8 data_020f7728[];

/* Pass the first position component by value as a word aggregate, and the
 * second as a scalar, matching func_0201ad50. Its pointer-shaped argument words
 * are converted explicitly here. Position groups keep each component pair
 * together while the common first component survives across slot iterations.
 */
static inline void create_positioned_object(void *destination, s32 entryWord, void *resource, Fixed first, Fixed second,
                                            u8 mode, s32 initializerWord) {
    func_0201ad50(destination, (void *) entryWord, resource, first, second.v, mode, (void *) initializerWord);
}

/* Populate the four-slot display from the manager's label IDs and row order.
 * Create the header and per-slot objects, append the shared heading, then
 * append three labels and three signed values for each occupied slot. Empty
 * slots still receive the common per-slot object. The caller-owned text state
 * is advanced through each emission. Object position components use
 * word wrappers in the inline adapter; variable row positions are scaled by
 * 4096.
 * Resource meanings, position axes and the header flag's role are unproved.
 */
void func_0209bef4(void *self, s32 *textState) {
    u8 slot;
    u32 order;
    s32 slotValue;
    u8 **managerRef;
    s32 objectIdOrTextY;
    Position header;
    Position slotPosition;
    Position common;
    func_02065600(self + 0x34, 4);
    func_02065658(self + 0x34, self + 0x2c);
    func_0206574c(self + 0x34, 0, -1);
    func_0206653c(0, 6, 5, 3, 4, 7, 0);
    func_0206574c(self + 0x34, 6, -1);
    *textState = func_02065be8(self + 0x34, *textState, 0x80, 8, func_02066708(HEADER_STRING_ID), 1);
    func_02065600(self + 0x34, 2);
    func_02065658(self + 0x34, self + 0x2c);
    func_0206574c(self + 0x34, 0, -1);
    func_0206653c(0, 6, 5, 3, 4, 7, 0);
    header.second.v = HEADER_SECOND_COMPONENT;
    header.first.v  = HEADER_FIRST_COMPONENT;
    create_positioned_object(self + 0xb4, 1, data_020f7728 + 6, header.first, header.second, 0, data_027e02a4);
    M2C_FIELD(M2C_FIELD(self, void **, 0xb4), u32 *, 8) |= 8;
    func_0201b2b8(M2C_FIELD(self, void **, 0xb4), 0x36a, 0);
    managerRef     = data_02107860;
    common.first.v = SLOT_FIRST_COMPONENT;
    /* The permutation chooses each slot's displayed row. */
    for (slot = 0; slot < SLOT_COUNT; slot++) {
        for (order = 0; order < SLOT_COUNT; order = (order + 1) & BYTE_MASK) {
            if (slot == managerRef[0][order + 0x30]) {
                break;
            }
        }
        slotValue = managerRef[0][slot + 0x2c];
        if (slotValue != EMPTY_LABEL) {
            /* Keep the signed index conversion for this position component. */
            slotPosition.second.v = ((int) order * ROW_SPACING + OBJECT_ROW_START) << POSITION_FRACTION_BITS;
            slotPosition.first.v  = SLOT_FIRST_COMPONENT;
            create_positioned_object(&((Obj *) self)->name[slot], slot + 7, data_020ef57c + slotValue * LABEL_RECORD_BYTES + 8,
                                     slotPosition.first, slotPosition.second, 0, data_027e02a4);
        } else {
            ((Obj *) self)->name[slot] = 0;
        }
        if (slot < 2) {
            objectIdOrTextY = slot + 3;
            func_02055ba4(0xf52, slot << 5, objectIdOrTextY << 4, 0x10, data_027e02a4);
        } else {
            objectIdOrTextY = slot + 3;
            func_02055ba4(0xf52, (slot + 1) << 5, objectIdOrTextY << 4, 0x10, data_027e02a4);
        }
        {
            common.second.v = (order * ROW_SPACING + OBJECT_ROW_START) << POSITION_FRACTION_BITS;
            create_positioned_object(&((Obj *) self)->line[slot], objectIdOrTextY, data_020f7720 + 2, common.first,
                                     common.second, 1, data_027e02a4);
        }
        if (slotValue != EMPTY_LABEL) {
            slotValue       = func_0209bc04(managerRef[0], slot);
            objectIdOrTextY = (order + 1) * ROW_SPACING;
            *textState = func_02065be8(self + 0x34, *textState, 0x34, objectIdOrTextY, func_02066708(VALUE_LABEL_1_ID), 0);
            *textState = func_02065be8(self + 0x34, *textState, 0x7f, objectIdOrTextY, func_02066708(VALUE_LABEL_2_ID), 0);
            *textState = func_02065be8(self + 0x34, *textState, 0xbb, objectIdOrTextY, func_02066708(VALUE_LABEL_3_ID), 0);
            func_02065600(self + 0x34, 4);
            /* Reuse the row index as the numeric text Y coordinate. */
            order = (s32) order * ROW_SPACING + VALUE_ROW_START;
            func_0209c2f0(self, textState, slotValue, 0x66, order);
            func_0209c2f0(self, textState, M2C_FIELD(managerRef[0] + slot * 2, s16 *, 0x1c), 0xa7, order);
            func_0209c2f0(self, textState, M2C_FIELD(managerRef[0] + slot * 2, s16 *, 0x24), 0xed, order);
            func_0206574c(self + 0x34, 0, -1);
            func_02065600(self + 0x34, 2);
        }
    }
}
