#include "m2c.h"

/*
 * Shape handler table; entry 0 is NNSi_G3dFuncSbc_SHP_InternalDefault.
 * Called with (obj, arg1, shape record, shape index).
 */
typedef void (*Handler)(void *, s32, u8 *, u32);

extern Handler data_021013a8[];

typedef struct {
    /* 0x00 */ u8 *cursor;       /* current position in the command stream */
    /* 0x04 */ u8 pad04[4];
    /* 0x08 */ u32 flags;
    /* 0x0C */ u8 pad0C[0xDC - 0x0C];
    /* 0xDC */ u8 *data;         /* resource block holding the shape index */
} Obj;

enum {
    FLAG_REQUIRED    = 0x1,      /* must be set for the handler to run */
    FLAG_BLOCK       = 0x2,         /* suppresses the handler */
    FLAG_NO_DISPATCH = 0x200, /* only advance the cursor */
};

enum {
    INDEX_TABLE_OFS  = 0x6,    /* u16 offset of the index table inside `data` */
    ENTRY_RECORD_OFS = 0x4,   /* u32 record offset (from `data`) in each entry */
    COMMAND_SIZE     = 2,         /* opcode byte + one operand byte */
};

/*
 * Two-byte shape command handler (in the dispatch table data_02101498).
 * When FLAG_REQUIRED is set and neither FLAG_BLOCK nor FLAG_NO_DISPATCH is,
 * it reads the operand byte cursor[1] as a shape index, looks it up in the
 * table inside `data` (first u16 = entry stride), resolves the entry's
 * record and calls the handler selected by the record's u16 tag with
 * (obj, arg1, record, index). The cursor always moves past the command.
 */
void func_01ffbe94(Obj *obj, s32 arg1) {
    u32 flags = obj->flags;

    if (!(flags & FLAG_NO_DISPATCH) && (flags & FLAG_REQUIRED) && !(flags & FLAG_BLOCK)) {
        u8 *data = obj->data;
        u32 idx  = obj->cursor[1];
        u8 *tbl  = data + *(u16 *) (data + INDEX_TABLE_OFS);
        u8 *rec  = data + *(u32 *) (tbl + *(u16 *) tbl * idx + ENTRY_RECORD_OFS);

        data_021013a8[*(u16 *) rec](obj, arg1, rec, idx);
    }
    obj->cursor += COMMAND_SIZE;
}
