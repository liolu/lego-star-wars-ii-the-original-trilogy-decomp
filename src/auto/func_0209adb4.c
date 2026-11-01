#include "m2c.h"

typedef struct {
    u8 pad[0xa4];
    u16 *tiles; /* u16 entries, 32 per row */
} Object;

enum {
    TILE_PIXELS = 8,  /* coordinates and sizes arrive in pixels */
    MAP_STRIDE  = 32,  /* entries per map row */
};

/*
 * Copies a rectangle of u16 map entries inside self->tiles.
 * Pixel coordinates/sizes are converted to cells by /8 (truncated to s16
 * for positions, u16 for sizes); rows are MAP_STRIDE entries wide.
 * Source at pixel (srcX, srcY), destination at (dstX, dstY), size
 * width x height pixels. Callers (func_0209a40c, func_0209a824) pass multiples of 8.
 */
void func_0209adb4(Object *self, s32 srcX, s32 srcY, s32 dstX, s16 dstY, u16 width, u16 height) {
    s16 srcCol = srcX / TILE_PIXELS;
    s16 srcRow = srcY / TILE_PIXELS;
    s16 dstCol = dstX / TILE_PIXELS;
    s16 dstRow = dstY / TILE_PIXELS;
    u16 cols   = width / TILE_PIXELS;
    u16 rows   = height / TILE_PIXELS;
    u16 *src   = &self->tiles[srcRow * MAP_STRIDE + srcCol];
    u16 *dst   = &self->tiles[dstRow * MAP_STRIDE + dstCol];
    u8 row;
    u8 col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            dst[row * MAP_STRIDE + col] = src[row * MAP_STRIDE + col];
        }
    }
}
