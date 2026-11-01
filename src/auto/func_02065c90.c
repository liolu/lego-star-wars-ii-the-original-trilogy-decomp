#include "m2c.h"

typedef struct Font {
    u8 unk0[8];
    u8 height;
    u8 unk9;
    u16 first;
    u16 last;
    u16 unkE;
    s32 *offsets;
    u8 *widths;
    u8 *glyphs;
} Font;

typedef struct TextCtx {
    s32 unk0;
    Font *font;
    Font *font2;
    s32 color;
    s32 fill;
    s32 unk14;
    s32 unk18;
    s32 map;
    s32 tiles;
    s32 mapWidth;
    s32 tileSize;
    s32 mode;
} TextCtx;

extern s32 func_0206640c(void *arg0, s32 arg1, s32 arg2, s32 arg3);
extern void func_020661e4(void *arg0, s32 *arg1, s32 arg2, s32 arg3);
extern void func_020662d8(void *arg0, u32 *arg1, s32 arg2, u16 *arg3, s32 arg4);
extern void func_0206636c(void *arg0, s32 *arg1, s32 arg2, u16 *arg3, s32 arg4, s32 arg5);
extern u8 data_021046fc[][64];

enum {
    MACRO_DEPTH   = 4,     /* nested "@n" macro expansions */
    FILL_NONE     = -1,      /* ctx->fill: start from the existing tiles */
    MODE_8BPP     = 1,       /* ctx->mode: one byte per pixel, else 4bpp */
    WIDE_CODE_MIN = 0xF0, /* first byte of a two-byte code (font2) */
    TILE_PX       = 8,         /* pixels per tile edge */
};

/*
 * Draws text into one tile column (TILE_PX pixels wide) at a time, starting
 * at pixel (x, y) of ctx's tile map, and returns the next free tile number.
 * For each column: buf is filled with ctx->fill (func_020661e4) or loaded
 * from the tiles already under the column (func_020662d8, when fill is
 * FILL_NONE); glyph pixels are drawn into it; then it is written to tile
 * numbers tile, tile + 1, ... and their map entries (func_0206636c).
 * "@n" inserts the 64-byte string data_021046fc[n - '1'] if it is not
 * empty (else "@n" is skipped); codes >= WIDE_CODE_MIN take a second byte
 * and use font2. Glyph pixels are 4-bit, two per byte; non-zero ones are
 * stored as pixel + color.
 */

s32 func_02065c90(TextCtx *ctx, s32 tile, s32 x, s32 y, u8 *text) {
    u8 *stack[MACRO_DEPTH]; /* macro return points */
    u32 buf[0x40];
    u8 *str;
    s32 inMacro;
    s32 depth = 0;
    u16 *map;      /* map entry of the current column */
    s32 tiles;     /* tile graphics base */
    Font *font;
    Font *font2;
    Font *font1;
    s32 cols;
    u8 *glyph;
    s32 shift;
    s32 color;
    s32 rows;      /* tiles per column */
    s32 glyphIdx;
    u32 c;
    u32 *bufPtr;

    map   = (u16 *) func_0206640c(ctx, ctx->map, x, y);
    font1 = ctx->font;
    tiles = ctx->tiles;
    y &= 7;
    rows  = (y + font1->height + 7) >> 3;
    font2 = ctx->font2;
    x &= 7;
    color  = ctx->color;
    bufPtr = buf;
    if (ctx->fill == FILL_NONE) {
        func_020662d8(ctx, bufPtr, rows, map, tiles);
    } else {
        func_020661e4(ctx, (s32 *) bufPtr, rows, ctx->fill);
    }
    str     = text;
    inMacro = 0;
    while (*str != 0 || inMacro != 0) {
        c = *str;
        if (inMacro != 0 && c == 0) {
            str = stack[--depth];
            if (depth == 0) {
                inMacro = 0;
            }
            c = *str;
            if (c == 0) {
                break;
            }
        }
        if (depth < MACRO_DEPTH && c == '@') {
            stack[depth] = str + 1;
            if (data_021046fc[str[1] - '1'][0] != 0) {
                str          = data_021046fc[str[1] - '1'];
                inMacro      = 1;
                stack[depth] = stack[depth] + 1;
                depth++;
            } else {
                str += 2;
                continue;
            }
        }
        c = *str;
        if (c >= WIDE_CODE_MIN) {
            c        = (u16) (c << 8) | *++str;
            font     = font2;
            glyphIdx = c - font->first;
        } else {
            font     = font1;
            glyphIdx = c - font->first;
        }
        if (glyphIdx < 0 || glyphIdx > font->last - font->first) {
            glyphIdx = 1;
        }
        glyph = font->glyphs + font->offsets[glyphIdx];
        shift = 0;
        cols  = font->widths[glyphIdx];
        while (cols--) {
            if (ctx->mode == MODE_8BPP) {
                u8 *dst = (u8 *) buf + y * 8 + x;
                s32 h   = font->height;
                while (h--) {
                    s32 pix;
                    if (shift == 8) {
                        glyph++;
                        shift = 0;
                    }
                    pix = (*glyph >> shift) & 0xF;
                    if (pix != 0) {
                        *dst = pix + color;
                    }
                    dst += 8;
                    shift += 4;
                }
            } else {
                s32 odd;
                u8 *dst = (u8 *) buf + (x + y * 8) / 2;
                s32 h   = font->height;
                if (h-- != 0) {
                    odd = x & 1;
                    do {
                        s32 pix;
                        if (shift == 8) {
                            glyph++;
                            shift = 0;
                        }
                        pix = (*glyph >> shift) & 0xF;
                        if (pix != 0) {
                            if (odd == 0) {
                                *dst = (*dst & 0xF0) | (pix + color);
                            } else {
                                *dst = (*dst & 0xF) | ((pix + color) << 4);
                            }
                        }
                        dst += 4;
                        shift += 4;
                    } while (h--);
                }
            }
            if (++x == TILE_PX) {
                x = 0;
                func_0206636c(ctx, (s32 *) bufPtr, rows, map, tiles, tile);
                tile += rows;
                map++;
                if (ctx->fill == FILL_NONE) {
                    func_020662d8(ctx, bufPtr, rows, map, tiles);
                } else {
                    func_020661e4(ctx, (s32 *) bufPtr, rows, ctx->fill);
                }
            }
        }
        str++;
    }
    if (x > 0) {
        func_0206636c(ctx, (s32 *) bufPtr, rows, map, tiles, tile);
        tile += rows;
    }
    return tile;
}
