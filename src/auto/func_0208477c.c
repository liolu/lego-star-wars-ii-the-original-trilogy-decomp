#include "m2c.h"

/* func_0208477c: draws one line of a multi-line text block into a two-tile-row
 * slot of a tilemap text layer.
 *
 * 1. If no string is pending (cur == NULL) and ids remain (index < count),
 *    fetch text id base + index via func_02066708 (copies it into a shared
 *    buffer, NULL if missing).  With colour_codes set, reset the text colour
 *    (func_0206574c arg1; -2 leaves arg2 unchanged) and parse a leading "^n"
 *    prefix: "^1" selects colour 9, any other "^n" colour 6.
 * 2. Map arg1 to a row in a 16-row ring (minus 3) and fill tilemap rows
 *    row*2 and row*2+1, columns 0..0x20, with 0 (func_020660e4 = DMA fill).
 * 3. Return early if skip_draw is set.
 * 4. Without colour_codes and once more than two ids were fetched, cut the
 *    text at LINE_WIDTH (func_020658e4), copy that piece, and justify it:
 *    spread the spare width (LINE_WIDTH - func_02065840 width) evenly over
 *    the spaces by inserting PAD_GLYPH after each space, then draw it centred
 *    at CENTRE_X.  A piece ending at '\n' is drawn unpadded, left-aligned at
 *    LEFT_X.  Otherwise hand the text to func_02065af8, which draws the next
 *    wrapped piece centred and advances cur itself (no character limit).
 * 5. Count the drawn line and drop cur once the string is used up.
 */

typedef struct Self {
    u8 pad0[0xCD8];
    u8 font[0x48];
    s32 count;
    s32 base;
    s32 draw_count;
    s32 index;
    u8 *cur;
    u8 pad1[0xEB8 - 0xD34];
    u8 skip_draw;
    u8 pad2;
    u8 colour_codes;
} Self;

enum {
    COLOUR_KEEP       = -2,
    COLOUR_DEFAULT    = 0x0,
    COLOUR_CODE_1     = 0x9,
    COLOUR_CODE_OTHER = 0x6,
    MAP_LAST_COLUMN   = 0x20,
    LINE_WIDTH        = 0xB8,
    CENTRE_X          = 0x80,
    LEFT_X            = 0x26,
    PAD_GLYPH         = 0xC6,
    ALIGN_LEFT        = 0,
    ALIGN_CENTRE      = 1,
};

s32 func_0205e6e0(u8 *dst, u8 *src, s32 len);
void func_0206574c(void *font, s32 a, s32 b);
s32 func_02065840(void *font, u8 *str);
u8 *func_020658e4(void *font, u8 *str, s32 width);
s32 func_02065af8(void *font, s32 a1, s32 x, s32 y, s32 width, u8 **str, s32 a6, s32 *a7);
void func_02065be8(void *font, s32 a1, s32 x, s32 y, u8 *str, s32 align);
void func_020660e4(void *font, s32 a1, s32 a2, s32 a3, u16 a4);
u8 *func_02066708(s32 id);

static inline u32 row_of(s32 n) {
    return ((n & 0xF) - 3) & 0xF;
}

void func_0208477c(Self *self, s32 arg1) {
    s32 char_budget;
    s32 x;
    u8 line[0x40];
    u8 justified[0xC0];
    u32 row;
    u32 tile;
    u8 *text;

    if (self->cur == NULL && self->index < self->count) {
        self->cur = func_02066708(self->base + self->index);
        if (self->colour_codes) {
            func_0206574c(self->font, COLOUR_DEFAULT, COLOUR_KEEP);
            text = self->cur;
            if (text[0] == '^') {
                if (text[1] == '1') {
                    func_0206574c(self->font, COLOUR_CODE_1, COLOUR_KEEP);
                } else {
                    func_0206574c(self->font, COLOUR_CODE_OTHER, COLOUR_KEEP);
                }
                self->cur += 2;
            }
        }
        self->index++;
    }
    row         = row_of(arg1);
    char_budget = 0x7FFFFFFF;
    tile        = row * 64;
    func_020660e4(self->font, 0, row * 2, MAP_LAST_COLUMN, 0);
    func_020660e4(self->font, 0, row * 2 + 1, MAP_LAST_COLUMN, 0);
    if (self->skip_draw) {
        return;
    }
    if (!self->colour_codes && self->index > 2) {
        u8 *str         = justified;
        u32 space_count = 0;
        s32 align       = ALIGN_CENTRE;
        u8 *line_end;
        x        = CENTRE_X;
        line_end = func_020658e4(self->font, self->cur, LINE_WIDTH);
        if (line_end != NULL) {
            u32 line_len = line_end - self->cur;
            u32 i;
            u32 pad_per_space;
            func_0205e6e0(line, self->cur, line_len);
            line[line_len] = 0;
            for (i = 0; i < line_len; i++) {
                if (line[i] == ' ') {
                    space_count++;
                }
            }
            pad_per_space = (u32) (LINE_WIDTH - func_02065840(self->font, line)) / space_count;
            self->cur     = line_end;
            if (*line_end == ' ' || *line_end == '\n') {
                self->cur++;
            }
            if (*line_end == '\n') {
                x     = LEFT_X;
                align = ALIGN_LEFT;
            }
            if (align == ALIGN_CENTRE && pad_per_space != 0) {
                u8 *dst = justified;
                u32 k;
                for (k = 0; k < line_len; k++) {
                    u8 c   = line[k];
                    *dst++ = c;
                    if (c == ' ') {
                        u32 j;
                        for (j = 0; j < pad_per_space; j++) {
                            *dst++ = PAD_GLYPH;
                        }
                    }
                }
                *dst = 0;
            } else {
                str = line;
            }
        } else {
            str = self->cur;
        }
        func_02065be8(self->font, tile + 1, x, row * 16, str, align);
    } else {
        func_02065af8(self->font, tile + 1, CENTRE_X, row * 16, LINE_WIDTH, &self->cur, 1, &char_budget);
    }
    self->draw_count++;
    if (*self->cur == 0) {
        self->cur = NULL;
    }
}
