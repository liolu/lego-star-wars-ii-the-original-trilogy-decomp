/* Image des deux ecrans en haute definition, dessinee dans des fils a part : le jeu garde sa cadence.
 *
 * A chaque VBlank le jeu copie registres, palettes, OAM et VRAM. Les moteurs 2D travaillent sur cette copie, en
 * 256x192 : c'est la definition de leurs dessins. La 2D est ensuite affinee (Scale2x) puis posee autour de la 3D,
 * calculee a la definition de la fenetre (gx3d.c). En format large, la 3D montre plus large et la 2D reste au centre. */
#include "nitro.h"

/* ------------------------------------------------------------------ banques VRAM */
#define KB(n) ((n) * 1024u)
static const u32 bank_size[9] = {KB(128), KB(128), KB(128), KB(128), KB(64), KB(16), KB(16), KB(32), KB(16)};
static const u32 lcdc_addr[9] = {0x06800000, 0x06820000, 0x06840000, 0x06860000, 0x06880000, 0x06890000, 0x06894000, 0x06898000, 0x068A0000};
static u8 bank_store[9][KB(128)];
static u32 bank_cpu[9];   /* adresse CPU (DS) actuelle de la banque, 0 si aucune */
static u8 bank_cnt[9];
static u8 *tex_slot[4];
static u8 *texpal_slot[6];
static u8 *bgext_a[4], *objext_a, *bgext_b[4], *objext_b;

static void map_special(int b, u32 mst, u32 ofs) {
    u8 *st = bank_store[b];
    switch (b) {
    case 0: case 1: case 2: case 3: if (mst == 3) tex_slot[ofs] = st; break;
    case 4:
        if (mst == 3) for (int i = 0; i < 4; i++) texpal_slot[i] = st + i * KB(16);
        if (mst == 4) for (int i = 0; i < 4; i++) bgext_a[i] = st + i * KB(8);
        if (mst == 5) objext_a = st;
        break;
    case 5: case 6:
        if (mst == 3) texpal_slot[(ofs & 1) + ((ofs >> 1) & 1) * 4] = st;
        if (mst == 4) { bgext_a[(ofs & 1) * 2] = st; bgext_a[(ofs & 1) * 2 + 1] = st + KB(8); }
        if (mst == 5) objext_a = st;
        break;
    case 7:
        if (mst == 2) for (int i = 0; i < 4; i++) bgext_b[i] = st + i * KB(8);
        break;
    case 8:
        if (mst == 3) objext_b = st;
        break;
    }
}

static u32 map_cpu(int b, u32 mst, u32 ofs) {
    if (mst == 0) return lcdc_addr[b];
    switch (b) {
    case 0: case 1:
        if (mst == 1) return 0x06000000 + ofs * KB(128);
        if (mst == 2) return 0x06400000 + (ofs & 1) * KB(128);
        break;
    case 2:
        if (mst == 1) return 0x06000000 + ofs * KB(128);
        if (mst == 4) return 0x06200000;
        break;
    case 3:
        if (mst == 1) return 0x06000000 + ofs * KB(128);
        if (mst == 4) return 0x06600000;
        break;
    case 4:
        if (mst == 1) return 0x06000000;
        if (mst == 2) return 0x06400000;
        break;
    case 5: case 6:
        if (mst == 1) return 0x06000000 + (ofs & 1) * KB(16) + ((ofs >> 1) & 1) * KB(64);
        if (mst == 2) return 0x06400000 + (ofs & 1) * KB(16) + ((ofs >> 1) & 1) * KB(64);
        break;
    case 7:
        if (mst == 1) return 0x06200000;
        break;
    case 8:
        if (mst == 1) return 0x06208000;
        if (mst == 2) return 0x06600000;
        break;
    }
    return 0;
}

static const u8 vram_reg[9] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x48, 0x49};

/* la banque suit sa position CPU : son contenu est recopie quand elle change de place */
void vram_cnt_changed(void) {
    for (int b = 0; b < 9; b++) {
        u8 v = REG8(IO + 0x200 + vram_reg[b]);
        if (v == bank_cnt[b]) continue;
        if (bank_cpu[b]) {
            nds_copy(bank_store[b], DS(bank_cpu[b]), bank_size[b]);
            nds_zero(DS(bank_cpu[b]), bank_size[b]);
            bank_cpu[b] = 0;
        }
        bank_cnt[b] = v;
    }
    for (int i = 0; i < 4; i++) { tex_slot[i] = 0; bgext_a[i] = 0; bgext_b[i] = 0; }
    for (int i = 0; i < 6; i++) texpal_slot[i] = 0;
    objext_a = objext_b = 0;
    for (int b = 0; b < 9; b++) {
        u8 v = bank_cnt[b];
        if (!(v & 0x80)) continue;
        u32 mst = v & 7, ofs = (v >> 3) & 3;
        u32 addr = map_cpu(b, mst, ofs);
        if (addr && !bank_cpu[b]) {
            nds_copy(DS(addr), bank_store[b], bank_size[b]);
            bank_cpu[b] = addr;
        } else if (!addr) {
            map_special(b, mst, ofs);
        }
    }
}

u8 *vram_tex_slot(int slot) { return tex_slot[slot & 3]; }
u8 *vram_texpal_slot(int slot) { return texpal_slot[slot % 6]; }

/* ------------------------------------------------------------------ copie prise au VBlank */
typedef struct {
    u8 io[0x2000], pal[0x800], oam[0x800];
    u8 bg_a[KB(512)], bg_b[KB(128)], obj_a[KB(256)], obj_b[KB(128)];
    u8 bgext_a[4][KB(8)], objext_a[KB(8)], bgext_b[4][KB(8)], objext_b[KB(8)];
    u8 has_bgext_a[4], has_objext_a, has_bgext_b[4], has_objext_b;
    u8 lcdc[4][KB(128)];
    u32 frame;
} Snapshot;
static Snapshot snap;
static volatile int worker_busy;
static OsEvent worker_event;

static void snapshot_take(void) {
    Snapshot *s = &snap;
    nds_copy(s->io, DS(IO), 0x2000);
    nds_copy(s->pal, DS(0x05000000u), 0x800);
    nds_copy(s->oam, DS(0x07000000u), 0x800);
    nds_copy(s->bg_a, DS(0x06000000u), KB(512));
    nds_copy(s->bg_b, DS(0x06200000u), KB(128));
    nds_copy(s->obj_a, DS(0x06400000u), KB(256));
    nds_copy(s->obj_b, DS(0x06600000u), KB(128));
    for (int i = 0; i < 4; i++) {
        s->has_bgext_a[i] = bgext_a[i] != 0;
        if (bgext_a[i]) nds_copy(s->bgext_a[i], bgext_a[i], KB(8));
        s->has_bgext_b[i] = bgext_b[i] != 0;
        if (bgext_b[i]) nds_copy(s->bgext_b[i], bgext_b[i], KB(8));
    }
    s->has_objext_a = objext_a != 0;
    if (objext_a) nds_copy(s->objext_a, objext_a, KB(8));
    s->has_objext_b = objext_b != 0;
    if (objext_b) nds_copy(s->objext_b, objext_b, KB(8));
    /* affichage direct de la VRAM (mode LCDC) : seulement s'il sert */
    u32 mode = (REG32(IO) >> 16) & 3;
    if (mode == 2) {
        u32 b = (REG32(IO) >> 18) & 3;
        nds_copy(s->lcdc[b], bank_cpu[b] == lcdc_addr[b] ? DS(lcdc_addr[b]) : bank_store[b], KB(128));
    }
    s->frame = nds_frame;
}

/* ------------------------------------------------------------------ une ligne 2D */
typedef struct {
    int main;
    u32 dispcnt;
    const u8 *io, *pal_bg, *pal_obj, *vram_bg, *vram_obj;
    u32 bg_mask, obj_mask;
    const u8 **bgext;
    const u8 *objext;
} Eng;

#define NOPIX 0xFFFF
#define PIX3D 0xFFFE
/* Pixel 2D avant affinage : couleur 15 bits (0-14), nature (16-17), force du melange sur la 3D (20-24) */
#define K_SOLID 0u
#define K_3D 1u    /* la 3D passe devant `couleur` */
#define K_OVER 2u  /* `couleur` melangee par-dessus la 3D */
#define PIXEL(color, kind, alpha) ((u32)(color) | ((kind) << 16) | ((u32)(alpha) << 20))

static u16 line_bg[4][256];
static u16 line_obj[256];
static u8 obj_prio[256], obj_semi[256], obj_win[256];
static u8 win_mask[256];

static inline u16 io16(const Eng *e, u32 r) { return *(const u16 *)(e->io + r); }
static inline u16 pal16(const u8 *p, u32 i) { return *(const u16 *)(p + i * 2) & 0x7FFF; }
static inline u16 bg16(const Eng *e, u32 off) { return *(const u16 *)(e->vram_bg + (off & e->bg_mask & ~1u)); }
static inline u8 bg8(const Eng *e, u32 off) { return e->vram_bg[off & e->bg_mask]; }
static inline u16 obj16(const Eng *e, u32 off) { return *(const u16 *)(e->vram_obj + (off & e->obj_mask & ~1u)); }
static inline u8 obj8(const Eng *e, u32 off) { return e->vram_obj[off & e->obj_mask]; }

static void bg_text(const Eng *e, int n, int y, u32 cnt) {
    u16 *out = line_bg[n];
    u32 hof = io16(e, 0x10 + n * 4) & 0x1FF, vof = io16(e, 0x12 + n * 4) & 0x1FF;
    u32 cb = ((cnt >> 2) & 15) * KB(16) + (e->main ? ((e->dispcnt >> 24) & 7) * KB(64) : 0);
    u32 sb = ((cnt >> 8) & 31) * KB(2) + (e->main ? ((e->dispcnt >> 27) & 7) * KB(64) : 0);
    u32 size = cnt >> 14;
    u32 w = (size & 1) ? 512 : 256, h = (size & 2) ? 512 : 256;
    int c256 = (cnt >> 7) & 1;
    u32 yy = (y + vof) & (h - 1);
    const u8 *ext = 0;
    if (c256 && (e->dispcnt >> 30) & 1) {
        int slot = n;
        if (n < 2 && (cnt >> 13) & 1) slot = n + 2;
        ext = e->bgext[slot];
    }
    for (int x = 0; x < 256; x++) {
        u32 xx = (x + hof) & (w - 1);
        u32 map = sb;
        u32 tx = xx >> 3, ty = yy >> 3;
        if (tx >= 32) { map += KB(2); tx -= 32; }
        if (ty >= 32) { map += (w == 512) ? KB(4) : KB(2); ty -= 32; }
        u16 ent = bg16(e, map + (ty * 32 + tx) * 2);
        u32 tile = ent & 0x3FF;
        u32 px = xx & 7, py = yy & 7;
        if (ent & 0x400) px = 7 - px;
        if (ent & 0x800) py = 7 - py;
        u32 c;
        if (c256) {
            c = bg8(e, cb + tile * 64 + py * 8 + px);
            if (!c) { out[x] = NOPIX; continue; }
            out[x] = ext ? *(const u16 *)(ext + ((ent >> 12) * 256 + c) * 2) & 0x7FFF : pal16(e->pal_bg, c);
        } else {
            u8 b = bg8(e, cb + tile * 32 + py * 4 + px / 2);
            c = (px & 1) ? b >> 4 : b & 15;
            if (!c) { out[x] = NOPIX; continue; }
            out[x] = pal16(e->pal_bg, (ent >> 12) * 16 + c);
        }
    }
}

static void aff_line(const Eng *e, int n, int y, s32 *sx, s32 *sy, s32 *dx, s32 *dy) {
    u32 base = 0x20 + (n - 2) * 16;
    s32 pa = (s16)io16(e, base), pb = (s16)io16(e, base + 2), pc = (s16)io16(e, base + 4), pd = (s16)io16(e, base + 6);
    s32 x0 = (s32)(*(const u32 *)(e->io + base + 8) << 4) >> 4, y0 = (s32)(*(const u32 *)(e->io + base + 12) << 4) >> 4;
    *sx = x0 + pb * y;
    *sy = y0 + pd * y;
    *dx = pa;
    *dy = pc;
}

static void bg_affine(const Eng *e, int n, int y, u32 cnt) {
    u16 *out = line_bg[n];
    u32 cb = ((cnt >> 2) & 15) * KB(16) + (e->main ? ((e->dispcnt >> 24) & 7) * KB(64) : 0);
    u32 sb = ((cnt >> 8) & 31) * KB(2) + (e->main ? ((e->dispcnt >> 27) & 7) * KB(64) : 0);
    u32 size = 128u << (cnt >> 14);
    int wrap = (cnt >> 13) & 1;
    s32 sx, sy, dx, dy;
    aff_line(e, n, y, &sx, &sy, &dx, &dy);
    for (int x = 0; x < 256; x++, sx += dx, sy += dy) {
        s32 tx = sx >> 8, ty = sy >> 8;
        if (wrap) { tx &= size - 1; ty &= size - 1; }
        else if (tx < 0 || ty < 0 || tx >= (s32)size || ty >= (s32)size) { out[x] = NOPIX; continue; }
        u8 tile = bg8(e, sb + (ty >> 3) * (size >> 3) + (tx >> 3));
        u8 c = bg8(e, cb + tile * 64 + (ty & 7) * 8 + (tx & 7));
        out[x] = c ? pal16(e->pal_bg, c) : NOPIX;
    }
}

static void bg_extended(const Eng *e, int n, int y, u32 cnt) {
    u16 *out = line_bg[n];
    u32 base = ((cnt >> 8) & 31);
    int wrap = (cnt >> 13) & 1;
    s32 sx, sy, dx, dy;
    aff_line(e, n, y, &sx, &sy, &dx, &dy);
    if (!(cnt & 0x80)) {
        /* carte 16 bits */
        u32 cb = ((cnt >> 2) & 15) * KB(16) + (e->main ? ((e->dispcnt >> 24) & 7) * KB(64) : 0);
        u32 sb = base * KB(2) + (e->main ? ((e->dispcnt >> 27) & 7) * KB(64) : 0);
        u32 size = 128u << (cnt >> 14);
        const u8 *ext = ((e->dispcnt >> 30) & 1) ? e->bgext[n] : 0;
        for (int x = 0; x < 256; x++, sx += dx, sy += dy) {
            s32 tx = sx >> 8, ty = sy >> 8;
            if (wrap) { tx &= size - 1; ty &= size - 1; }
            else if (tx < 0 || ty < 0 || tx >= (s32)size || ty >= (s32)size) { out[x] = NOPIX; continue; }
            u16 ent = bg16(e, sb + ((ty >> 3) * (size >> 3) + (tx >> 3)) * 2);
            u32 px = tx & 7, py = ty & 7;
            if (ent & 0x400) px = 7 - px;
            if (ent & 0x800) py = 7 - py;
            u8 c = bg8(e, cb + (ent & 0x3FF) * 64 + py * 8 + px);
            if (!c) { out[x] = NOPIX; continue; }
            out[x] = ext ? *(const u16 *)(ext + ((ent >> 12) * 256 + c) * 2) & 0x7FFF : pal16(e->pal_bg, c);
        }
        return;
    }
    static const u16 ws[4] = {128, 256, 512, 512}, hs[4] = {128, 256, 256, 512};
    u32 w = ws[cnt >> 14], h = hs[cnt >> 14];
    u32 addr = base * KB(16);
    int direct = (cnt >> 2) & 1;
    for (int x = 0; x < 256; x++, sx += dx, sy += dy) {
        s32 tx = sx >> 8, ty = sy >> 8;
        if (wrap) { tx &= w - 1; ty &= h - 1; }
        else if (tx < 0 || ty < 0 || tx >= (s32)w || ty >= (s32)h) { out[x] = NOPIX; continue; }
        if (direct) {
            u16 c = bg16(e, addr + (ty * w + tx) * 2);
            out[x] = (c & 0x8000) ? c & 0x7FFF : NOPIX;
        } else {
            u8 c = bg8(e, addr + ty * w + tx);
            out[x] = c ? pal16(e->pal_bg, c) : NOPIX;
        }
    }
}

static void render_objs(const Eng *e, int y) {
    static const u8 sw[4][4] = {{8, 16, 32, 64}, {16, 32, 32, 64}, {8, 8, 16, 32}, {8, 8, 8, 8}};
    static const u8 sh[4][4] = {{8, 16, 32, 64}, {8, 8, 16, 32}, {16, 32, 32, 64}, {8, 8, 8, 8}};
    const u8 *oam = snap.oam + (e->main ? 0 : 0x400);
    for (int x = 0; x < 256; x++) { line_obj[x] = NOPIX; obj_prio[x] = 4; obj_semi[x] = 0; obj_win[x] = 0; }
    if (!(e->dispcnt & 0x1000)) return;
    int map1d = (e->dispcnt >> 4) & 1;
    u32 boundary = map1d ? 32u << ((e->dispcnt >> 20) & 3) : 32;
    for (int i = 127; i >= 0; i--) {
        const u16 *attr = (const u16 *)(oam + i * 8);
        u16 a0 = attr[0], a1 = attr[1], a2 = attr[2];
        int affine = (a0 >> 8) & 1;
        if (!affine && (a0 & 0x200)) continue;
        u32 mode = (a0 >> 10) & 3, shape = a0 >> 14, size = a1 >> 14;
        if (shape == 3) continue;
        int w = sw[shape][size], h = sh[shape][size];
        int dbl = affine && (a0 & 0x200);
        int bw = dbl ? w * 2 : w, bh = dbl ? h * 2 : h;
        int oy = a0 & 0xFF;
        int ox = a1 & 0x1FF;
        if (ox >= 256) ox -= 512;
        int ly = (y - oy) & 0xFF;
        if (ly >= bh) continue;
        s32 pa = 256, pb = 0, pc = 0, pd = 256;
        if (affine) {
            const u16 *p = (const u16 *)(oam + ((a1 >> 9) & 31) * 32);
            pa = (s16)p[3]; pb = (s16)p[7]; pc = (s16)p[11]; pd = (s16)p[15];
        }
        u32 prio = (a2 >> 10) & 3, tile = a2 & 0x3FF, pal = a2 >> 12;
        int c256 = (a0 >> 13) & 1;
        for (int bx = 0; bx < bw; bx++) {
            int sx = ox + bx;
            if (sx < 0 || sx >= 256) continue;
            int tx, ty;
            if (affine) {
                int cx = bx - bw / 2, cy = ly - bh / 2;
                tx = ((pa * cx + pb * cy) >> 8) + w / 2;
                ty = ((pc * cx + pd * cy) >> 8) + h / 2;
                if (tx < 0 || ty < 0 || tx >= w || ty >= h) continue;
            } else {
                tx = (a1 & 0x1000) ? w - 1 - bx : bx;
                ty = (a1 & 0x2000) ? h - 1 - ly : ly;
            }
            u16 color;
            if (mode == 3) {
                u32 off;
                if ((e->dispcnt >> 6) & 1) {
                    u32 bnd = (e->dispcnt >> 22) & 1 ? 256 : 128;
                    off = tile * bnd + (ty * w + tx) * 2;
                } else if ((e->dispcnt >> 5) & 1) {
                    off = ((tile & 0x1F) * 8 + ((tile >> 5) & 0x1F) * 256 + ty * 256 + tx) * 2;
                } else {
                    off = ((tile & 0xF) * 8 + ((tile >> 4) & 0x1F) * 128 + ty * 128 + tx) * 2;
                }
                u16 c = obj16(e, off);
                if (!(c & 0x8000) || pal == 0) continue;
                color = c & 0x7FFF;
            } else if (c256) {
                u32 off;
                if (map1d) off = tile * boundary + (ty >> 3) * (w >> 3) * 64 + (tx >> 3) * 64;
                else off = (((tile & ~1u) + (ty >> 3) * 32 + (tx >> 3) * 2) & 0x3FF) * 32;
                u32 c = obj8(e, off + (ty & 7) * 8 + (tx & 7));
                if (!c) continue;
                if ((e->dispcnt >> 31) && e->objext) color = *(const u16 *)(e->objext + (pal * 256 + c) * 2) & 0x7FFF;
                else color = pal16(e->pal_obj, c);
            } else {
                u32 off;
                if (map1d) off = tile * boundary + (ty >> 3) * (w >> 3) * 32 + (tx >> 3) * 32;
                else off = ((tile + (ty >> 3) * 32 + (tx >> 3)) & 0x3FF) * 32;
                u8 b = obj8(e, off + (ty & 7) * 4 + (tx & 7) / 2);
                u32 c = (tx & 1) ? b >> 4 : b & 15;
                if (!c) continue;
                color = pal16(e->pal_obj, pal * 16 + c);
            }
            if (mode == 2) { obj_win[sx] = 1; continue; }
            if (prio <= obj_prio[sx] || line_obj[sx] == NOPIX) {
                line_obj[sx] = color;
                obj_prio[sx] = prio;
                obj_semi[sx] = mode == 1;
            }
        }
    }
}

static void compute_windows(const Eng *e, int y) {
    u32 d = e->dispcnt;
    u32 winin = io16(e, 0x48), winout = io16(e, 0x4A);
    int any = (d >> 13) & 7;
    for (int x = 0; x < 256; x++) win_mask[x] = any ? (u8)(winout & 0x3F) : 0x3F;
    if (!any) return;
    if ((d >> 15) & 1)
        for (int x = 0; x < 256; x++) if (obj_win[x]) win_mask[x] = (u8)((winout >> 8) & 0x3F);
    for (int w = 1; w >= 0; w--) {
        if (!((d >> (13 + w)) & 1)) continue;
        u32 hreg = io16(e, 0x40 + w * 2), vreg = io16(e, 0x44 + w * 2);
        int x1 = hreg >> 8, x2 = hreg & 0xFF, y1 = vreg >> 8, y2 = vreg & 0xFF;
        int in = y1 <= y2 ? (y >= y1 && y < y2) : (y >= y1 || y < y2);
        if (!in) continue;
        u32 m = (winin >> (w * 8)) & 0x3F;
        for (int x = 0; x < 256; x++) {
            int inside = x1 <= x2 ? (x >= x1 && x < x2) : (x >= x1 || x < x2);
            if (inside) win_mask[x] = (u8)m;
        }
    }
}

static inline u16 blend15(u16 a, u16 b, u32 eva, u32 evb) {
    u32 r = ((a & 31) * eva + (b & 31) * evb) >> 4;
    u32 g = (((a >> 5) & 31) * eva + ((b >> 5) & 31) * evb) >> 4;
    u32 bl = (((a >> 10) & 31) * eva + ((b >> 10) & 31) * evb) >> 4;
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (bl > 31) bl = 31;
    return (u16)(r | (g << 5) | (bl << 10));
}

static inline u16 lighten(u16 c, u32 evy) {
    u32 r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    return (u16)((r + ((31 - r) * evy >> 4)) | ((g + ((31 - g) * evy >> 4)) << 5) | ((b + ((31 - b) * evy >> 4)) << 10));
}

static inline u16 darken(u16 c, u32 evy) {
    u32 r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    return (u16)((r - (r * evy >> 4)) | ((g - (g * evy >> 4)) << 5) | ((b - (b * evy >> 4)) << 10));
}

/* Les 256x192 pixels d'un moteur ; renvoie s'il montre la 3D */
static int render_engine(const Eng *base, u32 *screen) {
    Eng e = *base;
    e.dispcnt = *(const u32 *)e.io;
    u32 dm = (e.dispcnt >> 16) & 3;
    int with3d = 0;
    if (dm == 0) {
        for (int i = 0; i < 256 * 192; i++) screen[i] = PIXEL(0x7FFF, K_SOLID, 0);
    } else if (e.main && dm == 2) {
        const u16 *src = (const u16 *)snap.lcdc[(e.dispcnt >> 18) & 3];
        for (int i = 0; i < 256 * 192; i++) screen[i] = PIXEL(src[i] & 0x7FFF, K_SOLID, 0);
    } else {
        u32 mode = e.dispcnt & 7;
        u16 backdrop = pal16(e.pal_bg, 0);
        u32 bldcnt = io16(&e, 0x50), bldalpha = io16(&e, 0x52), bldy = io16(&e, 0x54) & 31;
        u32 eva = bldalpha & 31, evb = (bldalpha >> 8) & 31;
        if (eva > 16) eva = 16;
        if (evb > 16) evb = 16;
        if (bldy > 16) bldy = 16;
        u32 bm = (bldcnt >> 6) & 3;
        int f3d = e.main && (e.dispcnt & 8) && (e.dispcnt & 0x100);
        with3d = f3d;
        for (int y = 0; y < 192; y++) {
            u32 cnts[4], prio[4];
            int on[4];
            for (int n = 0; n < 4; n++) {
                cnts[n] = io16(&e, 8 + n * 2);
                prio[n] = cnts[n] & 3;
                on[n] = (e.dispcnt >> (8 + n)) & 1;
                if (!on[n]) continue;
                if (n == 0 && f3d) {
                    for (int x = 0; x < 256; x++) line_bg[0][x] = PIX3D;
                    continue;
                }
                int kind = 0; /* texte */
                if (n == 2) kind = mode == 2 || mode == 4 ? 1 : mode == 5 ? 2 : 0;
                if (n == 3) kind = mode == 1 || mode == 2 ? 1 : mode >= 3 ? 2 : 0;
                if (mode == 6) kind = n == 2 ? 2 : 0;
                if (kind == 0) bg_text(&e, n, y, cnts[n]);
                else if (kind == 1) bg_affine(&e, n, y, cnts[n]);
                else bg_extended(&e, n, y, cnts[n]);
            }
            render_objs(&e, y);
            compute_windows(&e, y);
            u32 *out = screen + y * 256;
            for (int x = 0; x < 256; x++) {
                /* les deux couches du dessus, pour le melange */
                u16 c1 = backdrop, c2 = backdrop;
                int l1 = 5, l2 = 5;
                u32 mask = win_mask[x];
                int found = 0;
                for (u32 p = 0; p < 4 && found < 2; p++) {
                    if (line_obj[x] != NOPIX && obj_prio[x] == p && (mask & 16)) {
                        if (found == 0) { c1 = line_obj[x]; l1 = 4; } else { c2 = line_obj[x]; l2 = 4; }
                        found++;
                        if (found == 2) break;
                    }
                    for (int n = 0; n < 4 && found < 2; n++) {
                        if (!on[n] || prio[n] != p || !(mask & (1u << n)) || line_bg[n][x] == NOPIX) continue;
                        if (found == 0) { c1 = line_bg[n][x]; l1 = n; } else { c2 = line_bg[n][x]; l2 = n; }
                        found++;
                    }
                }
                if (c1 == PIX3D) {
                    /* la 3D devant : sa transparence laisse voir la couche suivante */
                    out[x] = PIXEL(c2 == PIX3D ? backdrop : c2, K_3D, 0);
                    continue;
                }
                int semi = l1 == 4 && obj_semi[x];
                int blends = (semi || ((mask & 32) && ((bldcnt >> l1) & 1) && bm == 1)) && ((bldcnt >> (8 + l2)) & 1);
                if (c2 == PIX3D && blends) {
                    out[x] = PIXEL(c1, K_OVER, eva);
                    continue;
                }
                u16 col = c1;
                if (blends) col = blend15(c1, c2, eva, evb);
                else if ((mask & 32) && ((bldcnt >> l1) & 1)) {
                    if (bm == 2) col = lighten(c1, bldy);
                    else if (bm == 3) col = darken(c1, bldy);
                }
                out[x] = PIXEL(col, K_SOLID, 0);
            }
        }
    }
    return with3d;
}

/* ------------------------------------------------------------------ fils de calcul */
#define MAX_THREADS 8
typedef void (*BandFn)(void *arg, int band, int bands);
static int pool_size;
static OsEvent pool_go[MAX_THREADS], pool_done[MAX_THREADS];
static volatile BandFn pool_fn;
static void *volatile pool_arg;

static void pool_thread(void *param) {
    int index = (int)(uptr)param;
    for (;;) {
        os_event_wait(pool_go[index]);
        pool_fn(pool_arg, index + 1, pool_size + 1);
        os_event_set(pool_done[index]);
    }
}

/* appelle fn(arg, 0..n-1, n) ; le fil courant fait la bande 0 */
void gfx_parallel(BandFn fn, void *arg) {
    pool_fn = fn;
    pool_arg = arg;
    for (int i = 0; i < pool_size; i++) os_event_set(pool_go[i]);
    fn(arg, 0, pool_size + 1);
    for (int i = 0; i < pool_size; i++) os_event_wait(pool_done[i]);
}

/* ------------------------------------------------------------------ assemblage */
#define MAX_F 4
static u32 raw[2][256 * 192];
static u32 fine_a[2][256 * MAX_F * 192 * MAX_F], fine_b[2][256 * 2 * 192 * 2], raw_seen[2][256 * 192];
static int fine_seen[2];
static u8 to8[32];

/* Scale2x : un pixel devient quatre, les marches d'escalier s'adoucissent sans flou */
typedef struct { const u32 *in; int w, h; u32 *out; } Scale;

static void scale2x_band(void *arg, int band, int bands) {
    const Scale *sc = arg;
    const u32 *in = sc->in;
    int w = sc->w, h = sc->h;
    u32 *out = sc->out;
    for (int y = h * band / bands; y < h * (band + 1) / bands; y++) {
        const u32 *up = in + (y ? y - 1 : y) * w, *cur = in + y * w, *dn = in + (y + 1 < h ? y + 1 : y) * w;
        u32 *o0 = out + y * 2 * w * 2, *o1 = o0 + w * 2;
        for (int x = 0; x < w; x++) {
            u32 b = up[x], d = cur[x ? x - 1 : x], e = cur[x], f = cur[x + 1 < w ? x + 1 : x], h2 = dn[x];
            if (b != h2 && d != f) {
                o0[x * 2] = d == b ? d : e;
                o0[x * 2 + 1] = b == f ? f : e;
                o1[x * 2] = d == h2 ? d : e;
                o1[x * 2 + 1] = h2 == f ? f : e;
            } else {
                o0[x * 2] = o0[x * 2 + 1] = o1[x * 2] = o1[x * 2 + 1] = e;
            }
        }
    }
}

static void scale2x(const u32 *in, int w, int h, u32 *out) {
    Scale sc = {in, w, h, out};
    gfx_parallel(scale2x_band, &sc);
}

#define FRAME_STRIDE 2688 /* present.c : MAX_W */
typedef struct {
    const u32 *fine; /* 2D affinee, F fois 256x192 */
    int f, k, w, h, margin, with3d;
    const u32 *color3d;
    u32 *out;
    u32 bright_mode, bright;
} Compose;

static inline u32 rgb15(u32 c) { return to8[c & 31] | ((u32)to8[(c >> 5) & 31] << 8) | ((u32)to8[(c >> 10) & 31] << 16); }

static inline u32 mix_rgb(u32 a, u32 b, u32 t) { /* t sur 256 : a*t + b*(256-t) */
    u32 u = 256 - t;
    u32 rb = (((a & 0xFF00FF) * t + (b & 0xFF00FF) * u) >> 8) & 0xFF00FF;
    u32 g = (((a & 0xFF00) * t + (b & 0xFF00) * u) >> 8) & 0xFF00;
    return rb | g;
}

static void compose_band(void *arg, int band, int bands) {
    const Compose *c = arg;
    int y0 = c->h * band / bands, y1 = c->h * (band + 1) / bands;
    int span = 256 * c->f;
    for (int y = y0; y < y1; y++) {
        int row = y * c->f / c->k;
        const u32 *fine = c->fine + row * span;
        const u32 *d3 = c->color3d + y * c->w;
        u32 *out = c->out + y * FRAME_STRIDE;
        for (int x = 0; x < c->w; x++) {
            int cx = x - c->margin;
            u32 p;
            if (cx >= 0 && cx < 256 * c->k) p = fine[cx * c->f / c->k];
            else p = c->with3d ? PIXEL(0, K_3D, 0) : PIXEL(0, K_SOLID, 0);
            u32 kind = (p >> 16) & 3;
            u32 rgb = rgb15(p);
            if (kind != K_SOLID && c->with3d) {
                u32 s = d3[x];
                u32 a = s >> 24;
                u32 back = kind == K_3D ? rgb : 0;
                u32 scene = a >= 255 ? (s & 0xFFFFFF) : mix_rgb(s, back, a + (a >> 7));
                rgb = kind == K_3D ? scene : mix_rgb(rgb, scene, ((p >> 20) & 31) * 16);
            }
            if (c->bright_mode == 1) {
                u32 t = c->bright * 16;
                rgb = mix_rgb(0xFFFFFF, rgb, t);
            } else if (c->bright_mode == 2) {
                rgb = mix_rgb(0, rgb, c->bright * 16);
            }
            out[x] = rgb | 0xFF000000u;
        }
    }
}

static void engine_output(const Eng *e, int index, int k, int w, int h, int margin, u32 *out, const u32 *color3d) {
    int with3d = render_engine(e, raw[index]);
    /* affinage : deux fois tant que c'est utile ; refait seulement si l'ecran a change */
    int f = k >= 3 ? 4 : k >= 2 ? 2 : 1;
    const u32 *fine = f == 4 ? fine_a[index] : f == 2 ? fine_b[index] : raw[index];
    int same = fine_seen[index] == f;
    for (int i = 0; same && i < 256 * 192; i++) same = raw[index][i] == raw_seen[index][i];
    if (!same) {
        if (f >= 2) scale2x(raw[index], 256, 192, fine_b[index]);
        if (f == 4) scale2x(fine_b[index], 512, 384, fine_a[index]);
        nds_copy(raw_seen[index], raw[index], sizeof raw[index]);
        fine_seen[index] = f;
    }
    u32 mb = *(const u16 *)(e->io + 0x6C);
    u32 bright = mb & 31;
    if (bright > 16) bright = 16;
    Compose c = {fine, f, k, w, h, margin, with3d, color3d, out, mb >> 14, bright};
    gfx_parallel(compose_band, &c);
}

/* ------------------------------------------------------------------ boucle du fil de rendu */
static u32 *back_top, *back_bottom;

static void render_once(void);

static void render_thread(void *arg) {
    for (;;) {
        os_event_wait(worker_event);
        render_once();
    }
}

static void render_once(void) {
    {
        int k, ds_width;
        present_config(&k, &ds_width);
        int w = ds_width * k, h = 192 * k;
        int margin = (w - 256 * k) / 2;
        present_back(&back_top, &back_bottom);
        const u32 *color3d = gx_render(w, h, k, margin);
        static const u8 *ext_a[4], *ext_b[4];
        for (int i = 0; i < 4; i++) {
            ext_a[i] = snap.has_bgext_a[i] ? snap.bgext_a[i] : 0;
            ext_b[i] = snap.has_bgext_b[i] ? snap.bgext_b[i] : 0;
        }
        Eng a = {1, 0, snap.io, snap.pal, snap.pal + 0x200, snap.bg_a, snap.obj_a, KB(512) - 1, KB(256) - 1, ext_a, snap.has_objext_a ? snap.objext_a : 0};
        Eng b = {0, 0, snap.io + 0x1000, snap.pal + 0x400, snap.pal + 0x600, snap.bg_b, snap.obj_b, KB(128) - 1, KB(128) - 1, ext_b, snap.has_objext_b ? snap.objext_b : 0};
        u32 pow = *(const u16 *)(snap.io + 0x304);
        u32 *sa = (pow >> 15) ? back_top : back_bottom, *sb = (pow >> 15) ? back_bottom : back_top;
        engine_output(&a, 0, k, w, h, margin, sa, color3d);
        engine_output(&b, 1, k, w, h, margin, sb, color3d);
        present_flip(w, h, snap.frame);
        worker_busy = 0;
    }
}

/* appele au VBlank par le fil du jeu */
void gfx_render(void) {
    static int started;
    if (!started) {
        started = 1;
        for (int i = 0; i < 32; i++) to8[i] = (u8)(i * 255 / 31);
        int cores = os_cpu_count();
        pool_size = cores > 4 ? (cores - 3 > MAX_THREADS - 1 ? MAX_THREADS - 1 : cores - 3) : 1;
        for (int i = 0; i < pool_size; i++) {
            pool_go[i] = os_event_create();
            pool_done[i] = os_event_create();
            os_thread_start(pool_thread, (void *)(uptr)i, 0x40000);
        }
        worker_event = os_event_create();
        if (!nds_fixed_clock) os_thread_start(render_thread, 0, 0x40000);
    }
    gx_begin_frame();
    if (nds_fixed_clock) { snapshot_take(); render_once(); return; } /* chaque image, dans l'ordre */
    if (worker_busy) return; /* le dessin precedent n'est pas fini : cette image n'est pas dessinee, le jeu continue */
    worker_busy = 1;
    snapshot_take();
    os_event_set(worker_event);
}
