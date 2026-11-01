#include "m2c.h"

enum {
    MAP_COLS         = 32,            /* entries per row of map and buf (32 x 32 u16 entries) */
    MAP_BYTES        = 0x800,
    MSG_NONE         = 0xFFFF,        /* title_msg / body_msg value for "no text" */
    TILE_PAL_SHIFT   = 12,      /* BG map entry bits 12-15: palette number */
    TILE_VFLIP       = 0x800,       /* BG map entry bit 11: vertical flip */
    GLYPH_TILE_FIRST = 0x37,  /* first tile after the frame set, given to the text renderer */
    ALIGN_LEFT       = 0,
    ALIGN_CENTER     = 1,         /* func_02065be8 subtracts half the text width */
};

/* Frame tile offsets from tile_base, named by where this routine places them. */
enum {
    TILE_FILL               = 0,            /* whole map and the cleared VRAM screen map */
    TILE_TOP_LEFT           = 1,
    TILE_TOP                = 2,
    TILE_TOP_RIGHT          = 3,
    TILE_TEXT_LEFT          = 4,       /* edges beside rows copied from buf */
    TILE_TEXT_BG            = 5,         /* buf is filled with it before text is rendered */
    TILE_TEXT_RIGHT         = 6,
    TILE_BOTTOM_LEFT        = 7,     /* last row when there is no panel */
    TILE_BOTTOM             = 8,
    TILE_BOTTOM_RIGHT       = 9,
    TILE_SEP_LEFT           = 0xA,      /* row under the title and row above the panel */
    TILE_SEP                = 0xB,
    TILE_SEP_RIGHT          = 0xC,
    TILE_PANEL_LEFT         = 0xD,    /* three panel rows filled further by func_0206c49c */
    TILE_PANEL              = 0xE,
    TILE_PANEL_RIGHT        = 0xF,
    TILE_PANEL_BOTTOM_LEFT  = 0x10,
    TILE_PANEL_BOTTOM       = 0x11,
    TILE_PANEL_BOTTOM_RIGHT = 0x12,
};

/* Window state; only the fields used here are named. */
typedef struct Window {
    u8 pad00[0x40];
    u16 *buf;                 /* map buffer the text renderer writes into */
    u8 pad44[0x10];
    s32 screen[4];            /* BG descriptor for the func_02053xxx VRAM helpers */
    s32 row;                  /* text row while drawing; window height in map rows on return */
    u8 pad68[8];
    s32 width;                /* window width in pixels */
    s32 panel_row;            /* first panel row, read by func_0206c49c */
    u8 pad78[8];
    u16 map[MAP_COLS][MAP_COLS];
    u8 pad880[4];
    u16 panel_mode;           /* below 3: draw the panel; also picks func_0206c49c's passes */
    u8 pad886[2];
    u16 palette;              /* BG palette number of the frame tiles */
    u16 tile_base;            /* tile number of the first frame tile */
    u16 body_msg;             /* message id of the body text, or MSG_NONE */
    u16 title_msg;            /* message id of the title, or MSG_NONE */
    u8 pad890[2];
    u8 text[2];               /* expanded message text (continues past this struct) */
} Window;

extern void func_0205e860(u16 *buf);
extern u16 *func_0205e818(s32 size, s32 arg1, s32 arg2);
extern s32 func_02053a28(s32 *screen);
extern s32 func_02053b50(s32 *screen);
extern s32 func_02053b14(s32 *screen);
extern void func_02065728(void *ctx, u16 *buf, s32 vram, s32 arg3, s32 arg4);
extern void *func_02066708(s32 id);
extern s32 func_02065798(void *ctx, void *str, u8 *dst);
extern s32 func_02065be8(void *ctx, s32 chr, s32 x, s32 y, u8 *text, s32 align);
extern s32 func_02065980(void *ctx, u8 *text, s32 width);
extern s32 func_02065a78(void *ctx, s32 chr, s32 x, s32 y, s32 width, u8 *text, s32 align);
extern void func_0206c49c(Window *w, u16 *map, s32 arg2);
extern s32 func_0205e6e0(u16 *dst, s32 src, s32 size);
extern void MIi_CpuClearFast(u32 data, void *destp, u32 size);

/* Map column of the window's right edge; columns 1 .. right - 1 are the inside. */
static inline s32 win_right_col(Window *w) {
    return ((w->width - 16) >> 3) + 2;
}

/* BG map entry: tile number plus palette in bits 12-15. */
static inline u16 win_attr(s32 chr, s32 palette) {
    return chr | (palette << TILE_PAL_SHIFT);
}

static inline u16 win_tile(Window *w, s32 n) {
    return win_attr(w->tile_base + n, w->palette);
}

/* One map row: left edge, repeated inside tile, right edge. */
static inline void win_fill_row(Window *w, u32 row, s32 left, s32 mid, s32 right) {
    s32 col;

    for (col = 1; col < win_right_col(w); col++) {
        w->map[row][col] = win_tile(w, mid);
    }
    w->map[row][0]                = win_tile(w, left);
    w->map[row][win_right_col(w)] = win_tile(w, right);
}

/* One map row taken from the rendered text in buf, between the text edge tiles. */
static inline void win_copy_row(Window *w, s32 row) {
    s32 col;
    u16 *src;

    src = &w->buf[row * MAP_COLS + 1];
    for (col = 1; col < win_right_col(w); col++) {
        w->map[row][col] = *src++;
    }
    w->map[row][0]                = win_tile(w, TILE_TEXT_LEFT);
    w->map[row][win_right_col(w)] = win_tile(w, TILE_TEXT_RIGHT);
}

/* Builds a framed text window in w->map.
 *
 * The title (title_msg) and body (body_msg) are rendered by the text renderer ctx into a fresh buf;
 * func_02065728 gives ctx that buffer, the BG character base in VRAM and the bytes per glyph tile
 * ((bpp8 + 1) << 5: 32 for 4bpp, 64 for 8bpp tiles). w->map then gets a top edge, the title rows
 * (1-2) and a separator, two map rows per body line, and either a separator, a three-row panel that
 * func_0206c49c completes and a panel bottom edge (panel_mode below 3), or a plain bottom edge.
 * Finally buf receives a copy of the BG screen map in VRAM (func_0205e6e0 copies its second argument
 * into its first), that VRAM map is cleared to TILE_FILL and w->row is set to the window height in
 * map rows.
 *
 * func_02065be8's result (its matched source is declared void) is the first glyph tile for the body.
 * Matching: win_attr keeps the palette as the second operand, win_fill_row takes an unsigned row
 * (row + 4 is not folded into a byte offset), and the three row loops use separate counters. */
void func_0206bc7c(Window *w, void *ctx) {
    s32 body_lines;
    s32 bpp8;
    s32 glyph_tile;
    s32 align;
    s32 x;
    s32 row;
    s32 title_row;
    s32 panel_line;
    s32 body_row;
    u32 fill;
    u16 entry;

    func_0205e860(w->buf);
    w->buf = func_0205e818(MAP_BYTES, 3, 4);
    entry  = win_tile(w, TILE_TEXT_BG);
    MIi_CpuClearFast(entry | (entry << 16), w->buf, MAP_BYTES);
    /* func_02053a28: 1 for 256-colour (8bpp) BG tiles */
    bpp8 = func_02053a28(w->screen) == 1;
    func_02065728(ctx, w->buf, func_02053b50(w->screen), 0x20, bpp8);
    glyph_tile = (u16) (w->tile_base + GLYPH_TILE_FIRST);
    w->row     = 1;
    if (w->title_msg != MSG_NONE) {
        /* centred title; text y is row * 8 + 3 pixels */
        func_02065798(ctx, func_02066708(w->title_msg), w->text);
        glyph_tile = func_02065be8(ctx, glyph_tile, w->width >> 1, w->row * 8 + 3, w->text, ALIGN_CENTER);
        w->row += 4;
    }
    body_lines = 0;
    if (w->body_msg != MSG_NONE) {
        /* wrapped to the width minus 16 pixels; left aligned at x 12 unless it fits on one line */
        func_02065798(ctx, func_02066708(w->body_msg), w->text);
        align      = ALIGN_LEFT;
        x          = 12;
        body_lines = func_02065980(ctx, w->text, w->width - 16);
        if (body_lines == 1) {
            x     = w->width >> 1;
            align = ALIGN_CENTER;
        }
        func_02065a78(ctx, glyph_tile, x, w->row * 8 + 3, w->width - 16, w->text, align);
    }
    entry = win_tile(w, TILE_FILL);
    fill  = entry | (entry << 16);
    row   = 1;
    MIi_CpuClearFast(fill, w->map, MAP_BYTES);
    win_fill_row(w, 0, TILE_TOP_LEFT, TILE_TOP, TILE_TOP_RIGHT);
    if (w->title_msg != MSG_NONE) {
        for (title_row = 1; title_row < 3; title_row++) {
            win_copy_row(w, title_row);
        }
        /* separator, then the same row flipped vertically */
        win_fill_row(w, 3, TILE_SEP_LEFT, TILE_SEP, TILE_SEP_RIGHT);
        win_fill_row(w, 4, TILE_VFLIP | TILE_SEP_LEFT, TILE_VFLIP | TILE_SEP, TILE_VFLIP | TILE_SEP_RIGHT);
        row += 4;
    }
    for (body_row = row; body_row < row + body_lines * 2; body_row++) {
        win_copy_row(w, body_row);
    }
    row += body_lines * 2;
    if (w->panel_mode < 3) {
        win_fill_row(w, row, TILE_SEP_LEFT, TILE_SEP, TILE_SEP_RIGHT);
        for (panel_line = row + 1; panel_line < row + 4; panel_line++) {
            win_fill_row(w, panel_line, TILE_PANEL_LEFT, TILE_PANEL, TILE_PANEL_RIGHT);
        }
        w->panel_row = row + 1;
        func_0206c49c(w, &w->map[0][0], 3);
        win_fill_row(w, row + 4, TILE_PANEL_BOTTOM_LEFT, TILE_PANEL_BOTTOM, TILE_PANEL_BOTTOM_RIGHT);
        row += 5;
    } else {
        win_fill_row(w, row, TILE_BOTTOM_LEFT, TILE_BOTTOM, TILE_BOTTOM_RIGHT);
        row += 1;
    }
    func_0205e6e0(w->buf, func_02053b14(w->screen), MAP_BYTES);
    MIi_CpuClearFast(fill, (void *) func_02053b14(w->screen), MAP_BYTES);
    w->row = row;
}
