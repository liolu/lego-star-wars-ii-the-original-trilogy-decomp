#include "m2c.h"

/* Handler table indexed by the u16 tag at the start of a resolved record. */
typedef void (*Handler)(void *, s32, u8 *, u32);

extern Handler data_021013b8[];

typedef struct {
    /* 0x00 */ u8 *cursor;       /* current position in the command stream */
    /* 0x04 */ u8 pad04[4];
    /* 0x08 */ u32 flags;
    /* 0x0C */ u8 pad0C[0xAD - 0x0C];
    /* 0xAD */ u8 last;          /* operand compared against when FLAG_CACHED */
    /* 0xAE */ u8 padAE[0xD8 - 0xAE];
    /* 0xD8 */ u8 *data;         /* resource block holding the record index */
} Obj;

enum {
    FLAG_FORCE       = 0x1,         /* dispatch even if the operand equals `last` */
    FLAG_CACHED      = 0x8,        /* `last` is valid for the equality skip */
    FLAG_NO_DISPATCH = 0x200, /* only advance the cursor */
};

enum {
    INDEX_OFFSET     = 0x4,       /* index area starts 4 bytes into `data` */
    INDEX_TABLE_OFS  = 0xA,    /* u16 offset of the table inside the index area */
    ENTRY_RECORD_OFS = 0x4,   /* u32 record offset (from `data`) in each entry */
    COMMAND_SIZE     = 2,         /* opcode byte + one operand byte */
};

/*
 * Two-byte command handler (entry 4 of the dispatch table data_02101498).
 * Reads the operand byte cursor[1] as an index. Unless FLAG_NO_DISPATCH is
 * set, or FLAG_CACHED is set without FLAG_FORCE and the index equals `last`,
 * it looks the index up in the table inside `data` (first u16 = entry
 * stride), resolves the entry's record and calls the handler selected by the
 * record's u16 tag with (obj, arg1, record, index). The cursor always moves
 * past the command.
 */
void func_01ffbd40(Obj *obj, s32 arg1) {
    u32 flags = obj->flags;

    if (!(flags & FLAG_NO_DISPATCH)) {
        u32 idx = obj->cursor[1];

        if ((flags & FLAG_FORCE) || !(flags & FLAG_CACHED) || idx != obj->last) {
            u8 *data = obj->data;
            u8 *list = data + INDEX_OFFSET;
            u8 *tbl  = list + *(u16 *) (data + INDEX_TABLE_OFS);
            u8 *rec  = data + *(u32 *) (tbl + *(u16 *) tbl * idx + ENTRY_RECORD_OFS);

            data_021013b8[*(u16 *) rec](obj, arg1, rec, idx);
        }
    }
    obj->cursor += COMMAND_SIZE;
}
