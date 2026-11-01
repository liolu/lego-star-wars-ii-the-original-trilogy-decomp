#include "m2c.h"

typedef struct {
    u16 *pixels;
    u16 width;
    u16 height;
    u8 frames;
    u8 unk9;
    u8 padA[2];
    s32 format;
} Image;

typedef struct {
    u8 pad0[6];
    u16 count;
    u8 pad8[2];
    u8 index;
    u8 num;
    u16 offsets[1];
} ResHeader;

typedef struct {
    u8 b[4];
} Bytes4;

typedef union {
    Bytes4 raw;
    struct {
        u32 pad : 18;
        u32 size1 : 2;
        u32 size0 : 2;
    } bits;
} Attr;

void func_0205a114(void *arg0);
void func_0205e860(void *arg0);
u8 *func_0205ab44(s32 arg0);
void func_0201aaf4(s32 arg0, s32 arg1, void *arg2);
void *func_0205e818(s32 size, s32 a1, s32 a2);
u32 func_02059efc(void *src, void *dst);
void func_0205a3b0(u16 *a0, s32 a1, u16 *a2, s32 a3);

static inline u8 *res_block(ResHeader *h, s32 i) {
    return (u8 *) h->offsets + h->offsets[i];
}

/*
 * Rebuilds self's pixel buffer from resource ids[1] (header) and ids[0]
 * (packed frame data): frees the old buffer, reads the size class from the
 * header's attribute word, looks the dimensions up with func_0201aaf4,
 * allocates width*height/2 bytes (4bpp) per frame (all frames when mode is
 * 1), then unpacks each frame into a scratch buffer and copies it in 8x8
 * tiles (func_0205a3b0) to a linear row-major layout of width/4 halfwords.
 */
void func_0205a3e8(Image *self, u16 *ids, s32 mode) {
    s32 count;
    ResHeader *hdr;
    Attr attr;
    u8 dims[2];
    u32 half;
    u32 size;
    u16 *tmp;
    s32 tilesX;
    s32 tilesY;
    s32 i;
    s32 y;
    s32 w;
    u16 *dst;
    s32 x;
    s32 tile;

    func_0205a114(self);
    func_0205e860(self->pixels);
    count = 1;
    hdr   = (ResHeader *) func_0205ab44(ids[1]);
    {
        /* 4-byte attribute word: block 0, past the 0xA-byte head and
           num 6-byte records, then index 2-byte steps; copied bytewise
           because the source is only byte aligned. */
        u8 *p;
        u8 *d;
        d = (u8 *) &attr;
        p = res_block(hdr, 0);
        p += 0xA;
        p    = p + hdr->num * 6 + hdr->index * 2;
        d[0] = p[0];
        d[1] = p[1];
        d[2] = p[2];
        d[3] = p[3];
    }
    func_0201aaf4(attr.bits.size0, attr.bits.size1, dims);
    half         = (dims[0] * dims[1]) / 2;
    self->width  = dims[0];
    self->height = dims[1];
    self->format = 3;
    size         = half;
    if (mode == 1) {
        /* mode 1: allocate every frame listed in the header */
        count        = hdr->count;
        size         = half * count;
        self->frames = count;
    }
    self->pixels = func_0205e818(size, 3, 4);
    tmp          = func_0205e818(half, 3, 4);
    tilesX       = self->width / 8;
    tilesY       = self->height / 8;
    w            = self->width;
    for (i = 0; i < count; i++) {
        u8 *frame = res_block(hdr, i);
        func_02059efc(func_0205ab44(ids[0]) + *(u16 *) (frame + 4), tmp);
        tile = 0;
        dst  = self->pixels + (half / 2) * i;
        for (y = 0; y < tilesY; y++) {
            for (x = 0; x < tilesX; x++) {
                func_0205a3b0(tmp, tile, &dst[x * 2 + y * (w / 4) * 8], w / 4);
                tile++;
            }
        }
    }
    func_0205e860(tmp);
}
