/* Moteur 3D du DS : commandes de geometrie -> liste de polygones -> image a la definition de la fenetre.
 * Le jeu remplit une liste ; a l'echange elle part au fil de rendu avec une copie des textures. */
#include "nitro.h"

typedef struct { s32 m[16]; } Mtx;
typedef struct { float x, y, z, w, r, g, b, s, t; u32 id; } Vtx;
typedef struct { Vtx v[10]; int n; u32 attr, tex, pal; int translucent; float ysort; Vtx src[4]; int src_n; u32 id; } Poly;

/* --interpolate, le mod 60 images/s : sans lui, rien de ce qu'il demande n'est calcule */
static inline int interp_on(void) {
    static int on = -1;
    if (on < 0) on = nds_arg("--interpolate", 0);
    return on;
}

#define MAX_POLYS 8192
static Poly polys[5][MAX_POLYS]; /* 3 listes qui tournent, la precedente pour l'interpolation, et l'image intermediaire */
static int poly_count[5];
static int build = 0;
static int swap_pending, swap_flags, render_flags;

static Mtx proj, pos, vec, tex, clip;
static Mtx proj_stack[2], pos_stack[32], vec_stack[32], tex_stack[2];
static int proj_sp, pos_sp, tex_sp, mtx_mode;
static s32 cur_v[3];
static u32 cur_color = 0x7FFF;
static s32 cur_s, cur_t, raw_s, raw_t;
static u32 next_attr, cur_attr, cur_tex, cur_pal;
static u32 dif_amb, spe_emi;
static s32 light_vec[4][3];
static u32 light_col[4];
static u8 shininess[128];
static int use_shine;
static int prim, prim_count;
static Vtx prim_v[4];
static int vp_x1, vp_y1, vp_x2 = 255, vp_y2 = 191;

static void m_id(Mtx *m) {
    nds_zero(m, sizeof(*m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 0x1000;
}

/* out = a * b, au sens du DS (lignes) */
static void m_mul(Mtx *out, const Mtx *a, const Mtx *b) {
    Mtx r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            s64 s = 0;
            for (int k = 0; k < 4; k++) s += (s64)a->m[i * 4 + k] * b->m[k * 4 + j];
            r.m[i * 4 + j] = (s32)(s >> 12);
        }
    *out = r;
}

static void update_clip(void) { m_mul(&clip, &pos, &proj); }

static Mtx *cur_mtx(void) {
    return mtx_mode == 0 ? &proj : mtx_mode == 3 ? &tex : &pos;
}

static void mul_cur(const Mtx *b) {
    if (mtx_mode == 2) {
        Mtx t;
        m_mul(&t, b, &vec);
        vec = t;
    }
    Mtx *c = cur_mtx();
    Mtx t;
    m_mul(&t, b, c);
    *c = t;
    if (mtx_mode != 3) update_clip();
}

static void load_cur(const Mtx *b) {
    if (mtx_mode == 2) vec = *b;
    *cur_mtx() = *b;
    if (mtx_mode != 3) update_clip();
}

/* ------------------------------------------------------------------ polygones */
static inline u32 c5(u32 c, int shift) { return (c >> shift) & 31; }

static void emit_poly(const Vtx *in, int n);

static void vertex(void) {
    s32 x = cur_v[0], y = cur_v[1], z = cur_v[2];
    Vtx o;
    o.x = ((s64)x * clip.m[0] + (s64)y * clip.m[4] + (s64)z * clip.m[8] + (s64)4096 * clip.m[12]) / (4096.0f * 4096.0f);
    o.y = ((s64)x * clip.m[1] + (s64)y * clip.m[5] + (s64)z * clip.m[9] + (s64)4096 * clip.m[13]) / (4096.0f * 4096.0f);
    o.z = ((s64)x * clip.m[2] + (s64)y * clip.m[6] + (s64)z * clip.m[10] + (s64)4096 * clip.m[14]) / (4096.0f * 4096.0f);
    o.w = ((s64)x * clip.m[3] + (s64)y * clip.m[7] + (s64)z * clip.m[11] + (s64)4096 * clip.m[15]) / (4096.0f * 4096.0f);
    o.r = (float)c5(cur_color, 0);
    o.g = (float)c5(cur_color, 5);
    o.b = (float)c5(cur_color, 10);
    u32 mode = cur_tex >> 30;
    if (mode == 3) {
        /* coordonnees de texture calculees depuis le sommet */
        o.s = (float)((((s64)x * tex.m[0] + (s64)y * tex.m[4] + (s64)z * tex.m[8]) >> 12) + raw_s * 16) / 256.0f;
        o.t = (float)((((s64)x * tex.m[1] + (s64)y * tex.m[5] + (s64)z * tex.m[9]) >> 12) + raw_t * 16) / 256.0f;
    } else {
        o.s = cur_s / 16.0f;
        o.t = cur_t / 16.0f;
    }
    /* ce que le jeu a donne, avant les matrices : un meme sommet garde cette valeur d'une image a l'autre */
    if (interp_on()) o.id = ((u32)x * 0x9E3779B1u) ^ ((u32)y * 0x85EBCA77u) ^ ((u32)z * 0xC2B2AE3Du) ^ ((u32)raw_s * 0x27D4EB2Fu) ^ ((u32)raw_t * 0x165667B1u);
    if (prim < 0) return;
    prim_v[prim_count++] = o;
    switch (prim) {
    case 0: /* triangles */
        if (prim_count == 3) { emit_poly(prim_v, 3); prim_count = 0; }
        break;
    case 1: /* quads */
        if (prim_count == 4) { emit_poly(prim_v, 4); prim_count = 0; }
        break;
    case 2: /* bande de triangles */
        if (prim_count == 3) {
            emit_poly(prim_v, 3);
            prim_v[0] = prim_v[1];
            prim_v[1] = prim_v[2];
            prim_count = 2;
            /* l'ordre alterne : on echange pour garder le meme sens */
            static int odd;
            (void)odd;
        }
        break;
    case 3: /* bande de quads */
        if (prim_count == 4) {
            Vtx q[4] = {prim_v[0], prim_v[1], prim_v[3], prim_v[2]};
            emit_poly(q, 4);
            prim_v[0] = prim_v[2];
            prim_v[1] = prim_v[3];
            prim_count = 2;
        }
        break;
    }
}

static int strip_flip;

static inline float plane(const Vtx *v, int p) {
    switch (p) {
    /* plus large que l'ecran : le format large montre ce que le DS coupait (MAX_WIDE / 256) */
    case 0: return v->w * 1.75f + v->x;
    case 1: return v->w * 1.75f - v->x;
    case 2: return v->w + v->y;
    case 3: return v->w - v->y;
    case 4: return v->w + v->z;
    default: return v->w - v->z;
    }
}

static void lerp(Vtx *o, const Vtx *a, const Vtx *b, float t) {
    o->x = a->x + (b->x - a->x) * t;
    o->y = a->y + (b->y - a->y) * t;
    o->z = a->z + (b->z - a->z) * t;
    o->w = a->w + (b->w - a->w) * t;
    o->r = a->r + (b->r - a->r) * t;
    o->g = a->g + (b->g - a->g) * t;
    o->b = a->b + (b->b - a->b) * t;
    o->s = a->s + (b->s - a->s) * t;
    o->t = a->t + (b->t - a->t) * t;
}

/* decoupage aux 6 plans ; a[] doit tenir 12 sommets. Rend le nombre de sommets, au plus 10, ou 0 si rien ne reste. */
static int clip_poly(Vtx *a, int n) {
    Vtx b[12];
    int na = n;
    for (int p = 0; p < 6; p++) {
        int nb = 0;
        for (int i = 0; i < na; i++) {
            const Vtx *c = &a[i], *d = &a[(i + 1) % na];
            float pc = plane(c, p), pd = plane(d, p);
            if (pc >= 0) b[nb++] = *c;
            if ((pc >= 0) != (pd >= 0) && nb < 11) lerp(&b[nb++], c, d, pc / (pc - pd));
            if (nb >= 11) break;
        }
        if (nb < 3) return 0;
        na = nb;
        for (int i = 0; i < na; i++) a[i] = b[i];
    }
    return na > 10 ? 10 : na;
}

static void emit_poly(const Vtx *in, int n) {
    Vtx a[12];
    int na = n;
    for (int i = 0; i < n; i++) a[i] = in[i];
    if (nds_cb.polygon) {
        /* nds_vertex : les 9 premiers champs de Vtx, dans le meme ordre */
        float mv[4][9];
        for (int i = 0; i < n; i++) nds_copy(mv[i], &a[i], sizeof mv[i]);
        if (nds_mods_polygon(mv, n, cur_attr, cur_tex, cur_pal)) return;
        for (int i = 0; i < n; i++) nds_copy(&a[i], mv[i], sizeof mv[i]);
    }
    if (prim == 2) {
        /* dans une bande, un triangle sur deux a le sens inverse */
        if (strip_flip) { Vtx t = a[1]; a[1] = a[2]; a[2] = t; }
        strip_flip = !strip_flip;
    }
    /* face cachee */
    {
        float ax = a[1].x * a[0].w - a[0].x * a[1].w, ay = a[1].y * a[0].w - a[0].y * a[1].w;
        float bx = a[2].x * a[0].w - a[0].x * a[2].w, by = a[2].y * a[0].w - a[0].y * a[2].w;
        float area = ax * by - ay * bx;
        /* signe : w peut etre negatif avant decoupage, on le compense */
        float sign = a[0].w * a[1].w * a[2].w;
        if (sign < 0) area = -area;
        int front = area > 0;
        if (front && !(cur_attr & 0x80)) return;
        if (!front && !(cur_attr & 0x40)) return;
    }
    Vtx source[4];
    if (interp_on()) for (int i = 0; i < n; i++) source[i] = a[i];
    na = clip_poly(a, n);
    if (!na) return;
    if (poly_count[build] >= MAX_POLYS) return;
    Poly *q = &polys[build][poly_count[build]++];
    q->n = na;
    for (int i = 0; i < na; i++) q->v[i] = a[i];
    if (interp_on()) {
        q->src_n = n;
        q->id = cur_attr ^ (cur_tex * 31u) ^ (cur_pal * 131u);
        for (int i = 0; i < n; i++) {
            q->src[i] = source[i];
            q->id += source[i].id * source[i].id; /* sans ordre : une bande retourne un triangle sur deux */
        }
    }
    q->attr = cur_attr;
    q->tex = cur_tex;
    q->pal = cur_pal;
    u32 alpha = (cur_attr >> 16) & 31;
    u32 fmt = (cur_tex >> 26) & 7;
    q->translucent = (alpha != 31 && alpha != 0) || fmt == 1 || fmt == 6;
}

static s32 sext10(u32 v) { return (s32)(v << 22) >> 22; }

static void normal(u32 p) {
    s32 n[3] = {sext10(p) << 3, sext10(p >> 10) << 3, sext10(p >> 20) << 3};
    if ((cur_tex >> 30) == 2) {
        /* coordonnees de texture depuis la normale */
        cur_s = (s32)(((s64)n[0] * tex.m[0] + (s64)n[1] * tex.m[4] + (s64)n[2] * tex.m[8]) >> 21) + raw_s;
        cur_t = (s32)(((s64)n[0] * tex.m[1] + (s64)n[1] * tex.m[5] + (s64)n[2] * tex.m[9]) >> 21) + raw_t;
    }
    s32 v[3];
    for (int i = 0; i < 3; i++) v[i] = (s32)(((s64)n[0] * vec.m[0 + i] + (s64)n[1] * vec.m[4 + i] + (s64)n[2] * vec.m[8 + i]) >> 12);
    int r = c5(spe_emi, 16), g = c5(spe_emi, 21), b = c5(spe_emi, 26);
    r <<= 0;
    for (int l = 0; l < 4; l++) {
        if (!(cur_attr & (1u << l))) continue;
        s32 d = -(s32)(((s64)light_vec[l][0] * v[0] + (s64)light_vec[l][1] * v[1] + (s64)light_vec[l][2] * v[2]) >> 12);
        s32 dif = d < 0 ? 0 : d > 0x1000 ? 0x1000 : d;
        /* reflet : direction moitie entre lumiere et vue (0,0,-1) */
        s32 hx = light_vec[l][0], hy = light_vec[l][1], hz = light_vec[l][2] - 0x1000;
        s32 sd = -(s32)(((s64)(hx / 2) * v[0] + (s64)(hy / 2) * v[1] + (s64)(hz / 2) * v[2]) >> 12);
        s32 spec = sd < 0 ? 0 : sd > 0x1000 ? 0x1000 : sd;
        spec = (s32)(((s64)spec * spec) >> 12);
        if (use_shine) spec = shininess[spec * 127 / 0x1000] << 4;
        u32 lc = light_col[l];
        r += (c5(spe_emi, 0) * c5(lc, 0) * spec >> 17) + (c5(dif_amb, 0) * c5(lc, 0) * dif >> 17) + (c5(dif_amb, 16) * c5(lc, 0) >> 5);
        g += (c5(spe_emi, 5) * c5(lc, 5) * spec >> 17) + (c5(dif_amb, 5) * c5(lc, 5) * dif >> 17) + (c5(dif_amb, 21) * c5(lc, 5) >> 5);
        b += (c5(spe_emi, 10) * c5(lc, 10) * spec >> 17) + (c5(dif_amb, 10) * c5(lc, 10) * dif >> 17) + (c5(dif_amb, 26) * c5(lc, 10) >> 5);
    }
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    cur_color = r | (g << 5) | (b << 10);
}

/* ------------------------------------------------------------------ commandes */
static const u8 param_count[256] = {
    [0x10] = 1, [0x12] = 1, [0x13] = 1, [0x14] = 1, [0x16] = 16, [0x17] = 12, [0x18] = 16, [0x19] = 12, [0x1A] = 9,
    [0x1B] = 3, [0x1C] = 3, [0x20] = 1, [0x21] = 1, [0x22] = 1, [0x23] = 2, [0x24] = 1, [0x25] = 1, [0x26] = 1,
    [0x27] = 1, [0x28] = 1, [0x29] = 1, [0x2A] = 1, [0x2B] = 1, [0x30] = 1, [0x31] = 1, [0x32] = 1, [0x33] = 1,
    [0x34] = 32, [0x40] = 1, [0x50] = 1, [0x60] = 1, [0x70] = 3, [0x71] = 2, [0x72] = 1,
};

static void write_clip_readback(void) {
    for (int i = 0; i < 16; i++) REG32(IO + 0x640 + i * 4) = (u32)clip.m[i];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) REG32(IO + 0x680 + (i * 3 + j) * 4) = (u32)vec.m[i * 4 + j];
}

static void exec_cmd(u32 cmd, const u32 *p);

static void exec(u32 cmd, const u32 *p) {
    if (nds_cb.gx) {
        u32 copy[32];
        u32 n = param_count[cmd];
        for (u32 i = 0; i < n && i < 32; i++) copy[i] = p[i];
        if (nds_mods_gx(cmd, copy, n)) return;
        exec_cmd(cmd, copy);
        return;
    }
    exec_cmd(cmd, p);
}

static void exec_cmd(u32 cmd, const u32 *p) {
    switch (cmd) {
    case 0x10: mtx_mode = p[0] & 3; break;
    case 0x11:
        if (mtx_mode == 0) proj_stack[proj_sp++ & 1] = proj;
        else if (mtx_mode == 3) tex_stack[tex_sp++ & 1] = tex;
        else { pos_stack[pos_sp & 31] = pos; vec_stack[pos_sp & 31] = vec; pos_sp++; }
        break;
    case 0x12: {
        s32 n = (s32)(p[0] << 26) >> 26;
        if (mtx_mode == 0) proj = proj_stack[--proj_sp & 1];
        else if (mtx_mode == 3) tex = tex_stack[--tex_sp & 1];
        else { pos_sp -= n; pos = pos_stack[pos_sp & 31]; vec = vec_stack[pos_sp & 31]; }
        if (mtx_mode != 3) update_clip();
        break;
    }
    case 0x13:
        if (mtx_mode == 0) proj_stack[0] = proj;
        else if (mtx_mode == 3) tex_stack[0] = tex;
        else { pos_stack[p[0] & 31] = pos; vec_stack[p[0] & 31] = vec; }
        break;
    case 0x14:
        if (mtx_mode == 0) proj = proj_stack[0];
        else if (mtx_mode == 3) tex = tex_stack[0];
        else { pos = pos_stack[p[0] & 31]; vec = vec_stack[p[0] & 31]; }
        if (mtx_mode != 3) update_clip();
        break;
    case 0x15: { Mtx m; m_id(&m); load_cur(&m); break; }
    case 0x16: { Mtx m; for (int i = 0; i < 16; i++) m.m[i] = (s32)p[i]; load_cur(&m); break; }
    case 0x17: {
        Mtx m; m_id(&m);
        for (int i = 0; i < 4; i++) for (int j = 0; j < 3; j++) m.m[i * 4 + j] = (s32)p[i * 3 + j];
        load_cur(&m);
        break;
    }
    case 0x18: { Mtx m; for (int i = 0; i < 16; i++) m.m[i] = (s32)p[i]; mul_cur(&m); break; }
    case 0x19: {
        Mtx m; m_id(&m);
        for (int i = 0; i < 4; i++) for (int j = 0; j < 3; j++) m.m[i * 4 + j] = (s32)p[i * 3 + j];
        mul_cur(&m);
        break;
    }
    case 0x1A: {
        Mtx m; m_id(&m);
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m.m[i * 4 + j] = (s32)p[i * 3 + j];
        mul_cur(&m);
        break;
    }
    case 0x1B: {
        /* l'echelle ne touche pas la matrice des vecteurs */
        Mtx m, t; m_id(&m);
        m.m[0] = (s32)p[0]; m.m[5] = (s32)p[1]; m.m[10] = (s32)p[2];
        Mtx *c = cur_mtx();
        m_mul(&t, &m, c);
        *c = t;
        if (mtx_mode != 3) update_clip();
        break;
    }
    case 0x1C: {
        Mtx m; m_id(&m);
        m.m[12] = (s32)p[0]; m.m[13] = (s32)p[1]; m.m[14] = (s32)p[2];
        mul_cur(&m);
        break;
    }
    case 0x20: cur_color = p[0] & 0x7FFF; break;
    case 0x21: normal(p[0]); break;
    case 0x22: {
        raw_s = (s16)p[0];
        raw_t = (s16)(p[0] >> 16);
        if ((cur_tex >> 30) == 1) {
            cur_s = (s32)(((s64)raw_s * tex.m[0] + (s64)raw_t * tex.m[4] + (s64)16 * tex.m[8] + (s64)16 * tex.m[12]) >> 12);
            cur_t = (s32)(((s64)raw_s * tex.m[1] + (s64)raw_t * tex.m[5] + (s64)16 * tex.m[9] + (s64)16 * tex.m[13]) >> 12);
        } else {
            cur_s = raw_s;
            cur_t = raw_t;
        }
        break;
    }
    case 0x23: cur_v[0] = (s16)p[0]; cur_v[1] = (s16)(p[0] >> 16); cur_v[2] = (s16)p[1]; vertex(); break;
    case 0x24: cur_v[0] = sext10(p[0]) << 6; cur_v[1] = sext10(p[0] >> 10) << 6; cur_v[2] = sext10(p[0] >> 20) << 6; vertex(); break;
    case 0x25: cur_v[0] = (s16)p[0]; cur_v[1] = (s16)(p[0] >> 16); vertex(); break;
    case 0x26: cur_v[0] = (s16)p[0]; cur_v[2] = (s16)(p[0] >> 16); vertex(); break;
    case 0x27: cur_v[1] = (s16)p[0]; cur_v[2] = (s16)(p[0] >> 16); vertex(); break;
    case 0x28: cur_v[0] += sext10(p[0]); cur_v[1] += sext10(p[0] >> 10); cur_v[2] += sext10(p[0] >> 20); vertex(); break;
    case 0x29: next_attr = p[0]; break;
    case 0x2A: cur_tex = p[0]; break;
    case 0x2B: cur_pal = p[0] & 0x1FFF; break;
    case 0x30:
        dif_amb = p[0];
        if (p[0] & 0x8000) cur_color = p[0] & 0x7FFF;
        break;
    case 0x31:
        spe_emi = p[0];
        use_shine = (p[0] >> 15) & 1;
        break;
    case 0x32: {
        int l = p[0] >> 30;
        s32 v[3] = {sext10(p[0]) << 3, sext10(p[0] >> 10) << 3, sext10(p[0] >> 20) << 3};
        for (int i = 0; i < 3; i++) light_vec[l][i] = (s32)(((s64)v[0] * vec.m[0 + i] + (s64)v[1] * vec.m[4 + i] + (s64)v[2] * vec.m[8 + i]) >> 12);
        break;
    }
    case 0x33: light_col[p[0] >> 30] = p[0] & 0x7FFF; break;
    case 0x34: for (int i = 0; i < 32; i++) nds_copy(&shininess[i * 4], &p[i], 4); break;
    case 0x40:
        cur_attr = next_attr;
        prim = p[0] & 3;
        prim_count = 0;
        strip_flip = 0;
        break;
    case 0x41: prim = -1; break;
    case 0x50: swap_pending = 1; swap_flags = p[0]; break;
    case 0x60:
        vp_x1 = p[0] & 255; vp_y1 = (p[0] >> 8) & 255;
        vp_x2 = (p[0] >> 16) & 255; vp_y2 = p[0] >> 24;
        break;
    case 0x71: {
        /* POS_TEST */
        s32 x = (s16)p[0], y = (s16)(p[0] >> 16), z = (s16)p[1];
        cur_v[0] = x; cur_v[1] = y; cur_v[2] = z;
        for (int i = 0; i < 4; i++)
            REG32(IO + 0x620 + i * 4) = (u32)(s32)(((s64)x * clip.m[i] + (s64)y * clip.m[4 + i] + (s64)z * clip.m[8 + i] + (s64)4096 * clip.m[12 + i]) >> 12);
        break;
    }
    case 0x72: {
        s32 n[3] = {sext10(p[0]) << 3, sext10(p[0] >> 10) << 3, sext10(p[0] >> 20) << 3};
        for (int i = 0; i < 3; i++)
            REG16(IO + 0x630 + i * 2) = (u16)(((s64)n[0] * vec.m[0 + i] + (s64)n[1] * vec.m[4 + i] + (s64)n[2] * vec.m[8 + i]) >> 12);
        break;
    }
    }
    if (cmd >= 0x10 && cmd <= 0x1C) write_clip_readback();
}

/* commande isolee ecrite sur son port */
static u32 port_buf[32];
static int port_n;

void gx_write(u32 reg, u32 value) {
    u32 cmd = (reg - (IO + 0x400)) / 4;
    if (cmd >= 0x80) return;
    u32 need = param_count[cmd];
    if (!need) { exec(cmd, 0); return; }
    port_buf[port_n++] = value;
    if ((u32)port_n >= need) { exec(cmd, port_buf); port_n = 0; }
}

/* FIFO empaquetee : un mot de 1 a 4 commandes, puis leurs parametres */
static u32 pack_cmds;
static int pack_left;
static u32 pack_params[32];
static int pack_have;

static void pack_next(void) {
    while (pack_left > 0) {
        u32 c = pack_cmds & 0xFF;
        u32 need = param_count[c];
        if ((u32)pack_have < need) return;
        exec(c, pack_params);
        pack_have = 0;
        pack_cmds >>= 8;
        pack_left--;
    }
}

void gx_fifo_word(u32 value) {
    if (pack_left == 0) {
        pack_cmds = value;
        pack_left = 4;
        /* les zeros de fin ne sont pas des commandes */
        while (pack_left > 1 && ((pack_cmds >> ((pack_left - 1) * 8)) & 0xFF) == 0) pack_left--;
        pack_have = 0;
        pack_next();
        return;
    }
    pack_params[pack_have++] = value;
    pack_next();
}

u32 gx_status(void) {
    return 0x06000002u | (pos_sp & 31) << 8 | (proj_sp & 1) << 13;
}

/* ------------------------------------------------------------------ images des textures, figees avec la liste */
#define KB(n) ((n) * 1024u)

/* Une liste prete : polygones, textures et registres du moment de l'echange */
typedef struct {
    u8 tex[4][KB(128)], has_tex[4];
    u8 pal[6][KB(16)], has_pal[6];
    u32 disp3d, clear, cdepth, aref, flags;
    u16 toon[32], edge[8];
    int vx1, vy1, vx2, vy2;
} ListState;
static ListState list_state[4];
static int list_ready = 1, list_shown = 2, list_old = 3;
#define LIST_MID 4
static volatile int list_fresh;
static volatile long list_lock;

/* ------------------------------------------------------------------ textures */
static const ListState *cur_state;

static inline float ffloor(float v) { s32 i = (s32)v; return (float)(i - (v < (float)i)); }

static inline const u8 *tex_ptr(u32 byte) {
    u32 slot = (byte >> 17) & 3;
    return cur_state->has_tex[slot] ? cur_state->tex[slot] + (byte & 0x1FFFF) : 0;
}
static inline const u8 *pal_ptr(u32 byte) {
    u32 slot = (byte >> 14) % 6;
    return cur_state->has_pal[slot] ? cur_state->pal[slot] + (byte & 0x3FFF) : 0;
}

/* renvoie 0 si transparent ; sinon alpha sur 5 bits en haut, couleur 555 */
static inline u32 texel(u32 texparam, u32 palbase, s32 s, s32 t) {
    u32 fmt = (texparam >> 26) & 7;
    u32 w = 8u << ((texparam >> 20) & 7), h = 8u << ((texparam >> 23) & 7);
    if (texparam & (1u << 16)) {
        if ((texparam & (1u << 18)) && (s & w)) s = ~s;
        s &= w - 1;
    } else s = s < 0 ? 0 : s >= (s32)w ? (s32)w - 1 : s;
    if (texparam & (1u << 17)) {
        if ((texparam & (1u << 19)) && (t & h)) t = ~t;
        t &= h - 1;
    } else t = t < 0 ? 0 : t >= (s32)h ? (s32)h - 1 : t;
    u32 base = (texparam & 0xFFFF) << 3;
    u32 c0trans = (texparam >> 29) & 1;
    u32 pb = palbase << 4;
    switch (fmt) {
    case 1: {
        const u8 *p = tex_ptr(base + t * w + s);
        if (!p) return 0;
        u32 idx = *p & 31, a = *p >> 5;
        const u8 *q = pal_ptr(pb + idx * 2);
        if (!q || !a) return 0;
        return (*(const u16 *)q & 0x7FFF) | ((a * 4 + a / 2) << 16);
    }
    case 2: {
        const u8 *p = tex_ptr(base + (t * w + s) / 4);
        if (!p) return 0;
        u32 idx = (*p >> (((t * w + s) & 3) * 2)) & 3;
        if (c0trans && !idx) return 0;
        const u8 *q = pal_ptr((palbase << 3) + idx * 2);
        return q ? (*(const u16 *)q & 0x7FFF) | (31u << 16) : 0;
    }
    case 3: {
        const u8 *p = tex_ptr(base + (t * w + s) / 2);
        if (!p) return 0;
        u32 idx = (*p >> (((t * w + s) & 1) * 4)) & 15;
        if (c0trans && !idx) return 0;
        const u8 *q = pal_ptr(pb + idx * 2);
        return q ? (*(const u16 *)q & 0x7FFF) | (31u << 16) : 0;
    }
    case 4: {
        const u8 *p = tex_ptr(base + t * w + s);
        if (!p) return 0;
        if (c0trans && !*p) return 0;
        const u8 *q = pal_ptr(pb + *p * 2);
        return q ? (*(const u16 *)q & 0x7FFF) | (31u << 16) : 0;
    }
    case 5: {
        u32 bx = s >> 2, by = t >> 2;
        u32 off = base + (by * (w >> 2) + bx) * 4;
        const u8 *p = tex_ptr(off);
        if (!p) return 0;
        u32 code = (p[t & 3] >> ((s & 3) * 2)) & 3;
        u32 slotoff = off & 0x1FFFF;
        u32 idxbase = 0x20000 + ((off >> 17) == 2 ? 0x10000 : 0) + slotoff / 2;
        const u8 *ip = tex_ptr(idxbase & ~1u);
        if (!ip) return 0;
        u16 info = *(const u16 *)ip;
        u32 pa = pb + (info & 0x3FFF) * 4;
        u32 mode = info >> 14;
#define PAL(i) (pal_ptr(pa + (i) * 2) ? *(const u16 *)pal_ptr(pa + (i) * 2) & 0x7FFF : 0)
        switch (code) {
        case 0: return PAL(0) | (31u << 16);
        case 1: return PAL(1) | (31u << 16);
        case 2:
            if (mode == 0 || mode == 2) return PAL(2) | (31u << 16);
            {
                u32 a = PAL(0), b = PAL(1), r;
                if (mode == 1) r = ((a & 31) + (b & 31)) / 2 | ((((a >> 5) & 31) + ((b >> 5) & 31)) / 2) << 5 | ((((a >> 10) & 31) + ((b >> 10) & 31)) / 2) << 10;
                else r = ((a & 31) * 5 + (b & 31) * 3) / 8 | ((((a >> 5) & 31) * 5 + ((b >> 5) & 31) * 3) / 8) << 5 | ((((a >> 10) & 31) * 5 + ((b >> 10) & 31) * 3) / 8) << 10;
                return r | (31u << 16);
            }
        default:
            if (mode == 0 || mode == 1) return 0;
            if (mode == 2) return PAL(3) | (31u << 16);
            {
                u32 a = PAL(0), b = PAL(1);
                u32 r = ((a & 31) * 3 + (b & 31) * 5) / 8 | ((((a >> 5) & 31) * 3 + ((b >> 5) & 31) * 5) / 8) << 5 | ((((a >> 10) & 31) * 3 + ((b >> 10) & 31) * 5) / 8) << 10;
                return r | (31u << 16);
            }
        }
    }
    case 6: {
        const u8 *p = tex_ptr(base + t * w + s);
        if (!p) return 0;
        u32 idx = *p & 7, a = *p >> 3;
        const u8 *q = pal_ptr(pb + idx * 2);
        if (!q || !a) return 0;
        return (*(const u16 *)q & 0x7FFF) | (a << 16);
    }
    case 7: {
        const u8 *p = tex_ptr(base + (t * w + s) * 2);
        if (!p) return 0;
        u16 c = *(const u16 *)p;
        if (!(c & 0x8000)) return 0;
        return (c & 0x7FFF) | (31u << 16);
    }
    }
    return 0;
}

/* ------------------------------------------------------------------ textures decodees, gardees tant que la VRAM ne change pas */
#define TEX_TABLE 4096
#define TEX_ARENA (40u * 1024 * 1024 / 4)
typedef struct { u32 key, pal; const u32 *pixels; } TexEntry;
static TexEntry tex_table[TEX_TABLE];
static u32 *tex_arena;
static u32 tex_used;
static u8 seen_tex[4][KB(128)], seen_pal[6][KB(16)], seen_has_tex[4], seen_has_pal[6];

static int same_words(const void *a, const void *b, u32 bytes) {
    const u32 *x = a, *y = b;
    for (u32 i = 0; i < bytes / 4; i++) if (x[i] != y[i]) return 0;
    return 1;
}

static void tex_cache_check(void) {
    const ListState *st = cur_state;
    int same = 1;
    for (int i = 0; i < 4 && same; i++)
        same = seen_has_tex[i] == st->has_tex[i] && (!st->has_tex[i] || same_words(seen_tex[i], st->tex[i], KB(128)));
    for (int i = 0; i < 6 && same; i++)
        same = seen_has_pal[i] == st->has_pal[i] && (!st->has_pal[i] || same_words(seen_pal[i], st->pal[i], KB(16)));
    if (same) return;
    for (int i = 0; i < 4; i++) {
        seen_has_tex[i] = st->has_tex[i];
        if (st->has_tex[i]) nds_copy(seen_tex[i], st->tex[i], KB(128));
    }
    for (int i = 0; i < 6; i++) {
        seen_has_pal[i] = st->has_pal[i];
        if (st->has_pal[i]) nds_copy(seen_pal[i], st->pal[i], KB(16));
    }
    nds_zero(tex_table, sizeof tex_table);
    tex_used = 0;
}

/* Une texture en RGBA 8 bits ; 0 si elle ne tient plus dans le cache. Appele avant le dessin, par un seul fil. */
static const u32 *tex_decoded(u32 texparam, u32 pal) {
    u32 key = texparam & 0x3FF0FFFFu; /* sans repetition ni miroir, qui se font a la lecture */
    u32 at = ((key * 2654435761u) ^ (pal * 40503u)) >> 20;
    for (int probe = 0; probe < TEX_TABLE; probe++, at = (at + 1) & (TEX_TABLE - 1)) {
        TexEntry *e = &tex_table[at];
        if (e->pixels) {
            if (e->key == key && e->pal == pal) return e->pixels;
            continue;
        }
        u32 w = 8u << ((key >> 20) & 7), h = 8u << ((key >> 23) & 7);
        if (tex_used + w * h > TEX_ARENA) return 0;
        u32 *out = tex_arena + tex_used;
        for (u32 t = 0; t < h; t++) {
            for (u32 s = 0; s < w; s++) {
                u32 c = texel(key, pal, s, t);
                u32 a = c >> 16;
                out[t * w + s] = a ? ((c & 31) * 255 / 31) | (((c >> 5) & 31) * 255 / 31) << 8 | (((c >> 10) & 31) * 255 / 31) << 16 | (a * 255 / 31) << 24 : 0;
            }
        }
        tex_used += w * h;
        e->key = key;
        e->pal = pal;
        e->pixels = out;
        return out;
    }
    return 0;
}

static inline s32 wrap_coord(s32 c, s32 size, int repeat, int flip) {
    if (repeat) {
        if (flip && (c & size)) c = ~c;
        return c & (size - 1);
    }
    return c < 0 ? 0 : c >= size ? size - 1 : c;
}

/* ------------------------------------------------------------------ rasterisation a la definition demandee */
#define MAX_W 2688
#define MAX_H 1152
#define STRIPS 48
static u32 *color_buf;
static float *z_buf;
static u8 *id_buf;

typedef struct { float sx, sy, iw, zw, rw, gw, bw, sw, tw; } SV;
typedef struct { SV sv[10]; const u32 *texels; s32 tex_w, tex_h; u8 rep_s, rep_t, flip_s, flip_t, alpha_tex; } Prep;
static Prep prep[MAX_POLYS];
static short prep_y0[MAX_POLYS], prep_y1[MAX_POLYS];
static u32 toon8[32], edge8[8];
typedef struct { int w, h, k, margin; const Poly *polys; int n; } Raster;
static volatile long next_strip;

static void prepare_polys(const Raster *ra) {
    const ListState *st = cur_state;
    float vw = (float)(st->vx2 - st->vx1 + 1), vh = (float)(st->vy2 - st->vy1 + 1);
    float left = (float)ra->margin + st->vx1 * ra->k;
    for (int n = 0; n < ra->n; n++) {
        const Poly *p = &ra->polys[n];
        Prep *q = &prep[n];
        float ymin = 1e9f, ymax = -1e9f;
        for (int i = 0; i < p->n; i++) {
            const Vtx *v = &p->v[i];
            SV *o = &q->sv[i];
            float iw = v->w > 1e-6f || v->w < -1e-6f ? 1.0f / v->w : 1e6f;
            o->sx = left + (v->x * iw + 1.0f) * 0.5f * vw * ra->k;
            o->sy = ((192 - st->vy1) - (v->y * iw + 1.0f) * 0.5f * vh) * ra->k;
            o->iw = iw;
            o->zw = v->z * iw;
            o->rw = v->r * iw;
            o->gw = v->g * iw;
            o->bw = v->b * iw;
            o->sw = v->s * iw;
            o->tw = v->t * iw;
            if (o->sy < ymin) ymin = o->sy;
            if (o->sy > ymax) ymax = o->sy;
        }
        prep_y0[n] = ymin < 0 ? 0 : ymin > 32000 ? 32000 : (short)ymin;
        prep_y1[n] = ymax < 0 ? -1 : ymax > 32000 ? 32000 : (short)ymax;
        u32 texp = p->tex, fmt = (texp >> 26) & 7;
        q->texels = (st->disp3d & 1) && fmt ? tex_decoded(texp, p->pal) : 0;
        q->tex_w = 8 << ((texp >> 20) & 7);
        q->tex_h = 8 << ((texp >> 23) & 7);
        q->rep_s = (texp >> 16) & 1;
        q->rep_t = (texp >> 17) & 1;
        q->flip_s = (texp >> 18) & 1;
        q->flip_t = (texp >> 19) & 1;
        q->alpha_tex = fmt == 1 || fmt == 6;
    }
}

static void draw_poly(const Poly *p, const Prep *q, const Raster *ra, int y0, int y1) {
    const ListState *st = cur_state;
    const SV *sv = q->sv;
    int full_x = st->vx1 == 0 && st->vx2 >= 255;
    int clip_x0 = full_x ? 0 : ra->margin + st->vx1 * ra->k;
    int clip_x1 = full_x ? ra->w - 1 : ra->margin + (st->vx2 + 1) * ra->k - 1;
    u32 attr = p->attr;
    u32 mode = (attr >> 4) & 3;
    if (mode == 3) return; /* ombres : non gerees */
    u32 raw_alpha = (attr >> 16) & 31;
    int wire = raw_alpha == 0;
    u32 palpha = wire ? 255 : raw_alpha * 255 / 31;
    u8 pid = (attr >> 24) & 63;
    u32 disp = st->disp3d;
    int blend_on = (disp >> 3) & 1;
    int alpha_test = (disp >> 2) & 1;
    int highlight = (disp >> 1) & 1;
    u32 aref = st->aref * 255 / 31;
    int update_depth = !p->translucent || ((attr >> 11) & 1);
    int depth_equal = (attr >> 14) & 1;
    int wmode = st->flags & 2;
    int smooth = ra->k > 1;
    const u32 *texels = q->texels;
    s32 tw = q->tex_w, th = q->tex_h;
    int wire_w = ra->k;
    for (int y = y0; y <= y1; y++) {
        float fy = y + 0.5f;
        float xl = 1e9f, xr = -1e9f;
        SV l, r;
        for (int i = 0; i < p->n; i++) {
            const SV *a = &sv[i], *b = &sv[(i + 1) % p->n];
            if ((a->sy <= fy) == (b->sy <= fy)) continue;
            float t = (fy - a->sy) / (b->sy - a->sy);
            float x = a->sx + (b->sx - a->sx) * t;
#define MIXV(o) o.iw = a->iw + (b->iw - a->iw) * t; o.zw = a->zw + (b->zw - a->zw) * t; o.rw = a->rw + (b->rw - a->rw) * t; o.gw = a->gw + (b->gw - a->gw) * t; o.bw = a->bw + (b->bw - a->bw) * t; o.sw = a->sw + (b->sw - a->sw) * t; o.tw = a->tw + (b->tw - a->tw) * t;
            if (x < xl) { xl = x; MIXV(l) }
            if (x > xr) { xr = x; MIXV(r) }
        }
        if (xl > xr) continue;
        int x0 = xl < clip_x0 ? clip_x0 : (int)(xl + 0.5f);
        int x1 = xr > clip_x1 + 1 ? clip_x1 : (int)(xr + 0.5f) - 1;
        if (x0 > x1) continue;
        float di = xr > xl ? 1.0f / (xr - xl) : 0;
        float diw = (r.iw - l.iw) * di, dzw = (r.zw - l.zw) * di, drw = (r.rw - l.rw) * di, dgw = (r.gw - l.gw) * di, dbw = (r.bw - l.bw) * di, dsw = (r.sw - l.sw) * di, dtw = (r.tw - l.tw) * di;
        float start = x0 + 0.5f - xl;
        float iw = l.iw + diw * start, zw = l.zw + dzw * start, rw = l.rw + drw * start, gw = l.gw + dgw * start, bw = l.bw + dbw * start, sw = l.sw + dsw * start, tcw = l.tw + dtw * start;
        u32 *colors = color_buf + y * ra->w;
        float *depths = z_buf + y * ra->w;
        u8 *ids = id_buf + y * ra->w;
        for (int x = x0; x <= x1; x++, iw += diw, zw += dzw, rw += drw, gw += dgw, bw += dbw, sw += dsw, tcw += dtw) {
            float w = 1.0f / iw;
            /* z/w varie lineairement a l'ecran ; mode W de l'echange : la profondeur est w */
            float z = wmode ? w : zw;
            if (depth_equal ? (z > depths[x] + 0.002f || z < depths[x] - 0.002f) : z >= depths[x]) continue;
            if (wire && x - x0 >= wire_w && x1 - x >= wire_w && y - y0 >= wire_w && y1 - y >= wire_w) continue;
            s32 cr = (s32)(rw * w * 8.2258f), cg = (s32)(gw * w * 8.2258f), cb = (s32)(bw * w * 8.2258f);
            cr = cr < 0 ? 0 : cr > 255 ? 255 : cr;
            cg = cg < 0 ? 0 : cg > 255 ? 255 : cg;
            cb = cb < 0 ? 0 : cb > 255 ? 255 : cb;
            if (mode == 2) {
                /* ombrage par la table de toon, indexee par le rouge du sommet */
                u32 toon = toon8[cr >> 3];
                if (highlight) {
                    s32 base = cr;
                    cr = base + (s32)(toon & 255);
                    cg = base + (s32)((toon >> 8) & 255);
                    cb = base + (s32)((toon >> 16) & 255);
                    cr = cr > 255 ? 255 : cr;
                    cg = cg > 255 ? 255 : cg;
                    cb = cb > 255 ? 255 : cb;
                } else {
                    cr = toon & 255;
                    cg = (toon >> 8) & 255;
                    cb = (toon >> 16) & 255;
                }
            }
            u32 a = palpha;
            if (texels) {
                float s = sw * w, t = tcw * w;
                u32 tr, tg, tb, ta;
                if (smooth) {
                    /* filtrage bilineaire ; les texels transparents ne deteignent pas sur leurs voisins */
                    float fs = s - 0.5f, ft = t - 0.5f;
                    float bs = ffloor(fs), bt = ffloor(ft);
                    u32 ux = (u32)((fs - bs) * 256.0f), uy = (u32)((ft - bt) * 256.0f);
                    s32 is = (s32)bs, it = (s32)bt;
                    s32 u0 = wrap_coord(is, tw, q->rep_s, q->flip_s), u1 = wrap_coord(is + 1, tw, q->rep_s, q->flip_s);
                    s32 v0 = wrap_coord(it, th, q->rep_t, q->flip_t) * tw, v1 = wrap_coord(it + 1, th, q->rep_t, q->flip_t) * tw;
                    u32 c00 = texels[v0 + u0], c01 = texels[v0 + u1], c10 = texels[v1 + u0], c11 = texels[v1 + u1];
                    if ((c00 & c01 & c10 & c11) >> 24 == 255) {
                        u32 vx = 256 - ux, vy = 256 - uy;
                        u32 top_rb = (((c00 & 0xFF00FF) * vx + (c01 & 0xFF00FF) * ux) >> 8) & 0xFF00FF;
                        u32 top_g = (((c00 & 0xFF00) * vx + (c01 & 0xFF00) * ux) >> 8) & 0xFF00;
                        u32 low_rb = (((c10 & 0xFF00FF) * vx + (c11 & 0xFF00FF) * ux) >> 8) & 0xFF00FF;
                        u32 low_g = (((c10 & 0xFF00) * vx + (c11 & 0xFF00) * ux) >> 8) & 0xFF00;
                        u32 rb = ((top_rb * vy + low_rb * uy) >> 8) & 0xFF00FF;
                        u32 gg = ((top_g * vy + low_g * uy) >> 8) & 0xFF00;
                        tr = rb & 255;
                        tg = gg >> 8;
                        tb = rb >> 16;
                        ta = 255;
                    } else {
                        const u32 cs[4] = {c00, c01, c10, c11};
                        const u32 wt[4] = {(256 - ux) * (256 - uy), ux * (256 - uy), (256 - ux) * uy, ux * uy};
                        u32 sum_a = 0, sum_r = 0, sum_g = 0, sum_b = 0;
                        for (int i = 0; i < 4; i++) {
                            u32 wa = (wt[i] >> 8) * (cs[i] >> 24) >> 8;
                            sum_a += wa;
                            sum_r += wa * (cs[i] & 255);
                            sum_g += wa * ((cs[i] >> 8) & 255);
                            sum_b += wa * ((cs[i] >> 16) & 255);
                        }
                        if (!sum_a) continue;
                        tr = sum_r / sum_a;
                        tg = sum_g / sum_a;
                        tb = sum_b / sum_a;
                        ta = sum_a > 255 ? 255 : sum_a;
                    }
                } else {
                    s32 u = wrap_coord((s32)ffloor(s), tw, q->rep_s, q->flip_s), v = wrap_coord((s32)ffloor(t), th, q->rep_t, q->flip_t);
                    u32 c = texels[v * tw + u];
                    if (!c) continue;
                    tr = c & 255;
                    tg = (c >> 8) & 255;
                    tb = (c >> 16) & 255;
                    ta = c >> 24;
                }
                /* bord d'une texture a trous : net, sans halo */
                if (!q->alpha_tex) {
                    if (ta < 128) continue;
                    ta = 255;
                }
                if (mode == 1) {
                    cr = (tr * ta + cr * (255 - ta)) / 255;
                    cg = (tg * ta + cg * (255 - ta)) / 255;
                    cb = (tb * ta + cb * (255 - ta)) / 255;
                } else {
                    cr = (cr * tr + 127) / 255;
                    cg = (cg * tg + 127) / 255;
                    cb = (cb * tb + 127) / 255;
                    a = (a * ta + 127) / 255;
                }
            }
            if (alpha_test && a <= aref) continue;
            if (!a) continue;
            u32 d = colors[x];
            if (blend_on && a < 255 && (d >> 24)) {
                u32 inv = 255 - a;
                cr = (cr * a + (d & 255) * inv) / 255;
                cg = (cg * a + ((d >> 8) & 255) * inv) / 255;
                cb = (cb * a + ((d >> 16) & 255) * inv) / 255;
                if ((d >> 24) > a) a = d >> 24;
            }
            colors[x] = (u32)cr | ((u32)cg << 8) | ((u32)cb << 16) | (a << 24);
            if (update_depth) depths[x] = z;
            ids[x] = pid;
        }
    }
}

static void clear_band(void *arg, int band, int bands) {
    const Raster *ra = arg;
    const ListState *st = cur_state;
    u32 c = st->clear;
    u32 color = ((c & 31) * 255 / 31) | (((c >> 5) & 31) * 255 / 31) << 8 | (((c >> 10) & 31) * 255 / 31) << 16 | (((c >> 16) & 31) * 255 / 31) << 24;
    u8 clear_id = (c >> 24) & 63;
    float zclear = st->cdepth / 16383.5f - 1.0f;
    if (st->cdepth == 0x7FFF) zclear = 1.0001f;
    if (st->flags & 2) zclear = 1e30f;
    int y0 = ra->h * band / bands, y1 = ra->h * (band + 1) / bands;
    for (int i = y0 * ra->w; i < y1 * ra->w; i++) {
        color_buf[i] = color;
        z_buf[i] = zclear;
        id_buf[i] = clear_id;
    }
}

/* Chaque fil prend la prochaine bande libre : les bandes chargees ne retardent pas tout le monde */
static void raster_band(void *arg, int band, int bands) {
    const Raster *ra = arg;
    for (;;) {
        int strip = __sync_fetch_and_add(&next_strip, 1);
        if (strip >= STRIPS) return;
        int y0 = ra->h * strip / STRIPS, y1 = ra->h * (strip + 1) / STRIPS - 1;
        for (int pass = 0; pass < 2; pass++) {
            for (int i = 0; i < ra->n; i++) {
                if (prep_y1[i] < y0 || prep_y0[i] > y1 || ra->polys[i].translucent != pass) continue;
                draw_poly(&ra->polys[i], &prep[i], ra, prep_y0[i] > y0 ? prep_y0[i] : y0, prep_y1[i] < y1 ? prep_y1[i] : y1);
            }
        }
    }
}

/* contours : un pixel dont le voisin plus lointain appartient a un autre polygone */
static void edge_band(void *arg, int band, int bands) {
    const Raster *ra = arg;
    int y0 = ra->h * band / bands, y1 = ra->h * (band + 1) / bands;
    int step = ra->k > 2 ? ra->k / 2 : 1;
    if (y0 < step) y0 = step;
    if (y1 > ra->h - step) y1 = ra->h - step;
    for (int y = y0; y < y1; y++) {
        for (int x = step; x < ra->w - step; x++) {
            int at = y * ra->w + x;
            u8 id = id_buf[at];
            float z = z_buf[at];
            int n[4] = {at - step, at + step, at - step * ra->w, at + step * ra->w};
            for (int i = 0; i < 4; i++) {
                if (id_buf[n[i]] != id && z < z_buf[n[i]]) {
                    color_buf[at] = (color_buf[at] & 0xFF000000u) | edge8[id >> 3];
                    break;
                }
            }
        }
    }
}

/* --interpolate : image intermediaire entre les deux dernieres listes. Les polygones se retrouvent par leurs sommets d'origine, ceux qui n'ont
 * pas d'equivalent ou qui sautent trop loin (changement de plan) restent ou ils sont dans la nouvelle liste. */
#define INTERP_BUCKETS 16384
#define INTERP_MAX_JUMP 0.6f

static inline int same_poly(const Poly *a, const Poly *b) { return a->id == b->id && a->src_n == b->src_n && a->attr == b->attr && a->tex == b->tex && a->pal == b->pal; }

static inline float abs_f(float v) { return v < 0 ? -v : v; }

/* Le milieu se calcule sur les sommets d'avant le decoupage : decoupes ensuite, les polygones voisins restent joints au bord de l'ecran. */
static int interp_pair(Poly *out, const Poly *old, const Poly *now, float scene_w) {
    Vtx a[12];
    for (int i = 0; i < now->src_n; i++) {
        const Vtx *from = &old->src[i], *to = &now->src[i];
        /* un sommet proche de la camera, ou derriere elle, bouge beaucoup sans que ce soit un saut : on le juge a l'echelle de la scene */
        float far = abs_f(from->w) > abs_f(to->w) ? abs_f(from->w) : abs_f(to->w);
        float reach = (far > scene_w ? far : scene_w) * INTERP_MAX_JUMP;
        if (abs_f(from->x - to->x) > reach || abs_f(from->y - to->y) > reach) return 0;
        a[i] = *to;
        a[i].x = (from->x + to->x) * 0.5f;
        a[i].y = (from->y + to->y) * 0.5f;
        a[i].z = (from->z + to->z) * 0.5f;
        a[i].w = (from->w + to->w) * 0.5f;
    }
    int n = clip_poly(a, now->src_n);
    if (!n) return 0;
    out->n = n;
    for (int i = 0; i < n; i++) out->v[i] = a[i];
    return 1;
}

/* Rend 1 si assez de polygones se sont retrouves pour que l'image intermediaire ait un sens. Un meme modele peut etre dessine plusieurs fois :
 * chaque polygone de l'ancienne liste ne sert qu'une fois, dans l'ordre de dessin. */
static int interp_midpoint(void) {
    static int head[INTERP_BUCKETS], next[MAX_POLYS];
    const Poly *old = polys[list_old], *now = polys[list_shown];
    int old_n = poly_count[list_old], now_n = poly_count[list_shown];
    Poly *out = polys[LIST_MID];
    int moved = 0;
    float scene_w = 0;
    for (int i = 0; i < INTERP_BUCKETS; i++) head[i] = -1;
    for (int i = old_n - 1; i >= 0; i--) {
        u32 bucket = old[i].id & (INTERP_BUCKETS - 1);
        next[i] = head[bucket];
        head[bucket] = i;
    }
    for (int i = 0; i < now_n; i++) scene_w += abs_f(now[i].v[0].w);
    if (now_n) scene_w /= now_n;
    for (int i = 0; i < now_n; i++) {
        nds_copy(&out[i], &now[i], sizeof out[i]); /* pas d'affectation : gcc y appellerait memcpy, absent de certains jeux */
        int *link = &head[now[i].id & (INTERP_BUCKETS - 1)];
        while (*link >= 0 && !same_poly(&old[*link], &now[i])) link = &next[*link];
        if (*link < 0) continue;
        moved += interp_pair(&out[i], &old[*link], &now[i], scene_w);
        *link = next[*link];
    }
    poly_count[LIST_MID] = now_n;
    return moved > now_n / 4;
}

/* Dans le fil de rendu : l'image 3D a cette definition, refaite si la liste ou la definition a change. */
const u32 *gx_render(int w, int h, int k, int margin) {
    static int last_w, last_h;
    if (!color_buf) {
        color_buf = os_mem_alloc(0, MAX_W * MAX_H * 4);
        z_buf = os_mem_alloc(0, MAX_W * MAX_H * 4);
        id_buf = os_mem_alloc(0, MAX_W * MAX_H);
        tex_arena = os_mem_alloc(0, TEX_ARENA * 4);
    }
    static int calls_since, last_gap, final_pending;
    int interp = interp_on();
    while (__sync_lock_test_and_set(&list_lock, 1)) os_yield();
    int fresh = list_fresh;
    if (fresh) {
        int t = interp ? list_old : list_shown;
        if (interp) list_old = list_shown;
        list_shown = list_ready;
        list_ready = t;
        list_fresh = 0;
    }
    __sync_lock_release(&list_lock);
    calls_since++;
    if (fresh) {
        last_gap = calls_since;
        calls_since = 0;
    }
    /* un jeu a 30 images/s : a son image neuve, on montre d'abord le milieu du chemin depuis la precedente, puis l'image elle-meme */
    int mid = interp && fresh && last_gap >= 2 && last_gap <= 3 && interp_midpoint();
    int finish = !fresh && final_pending;
    final_pending = mid;
    if (!fresh && !finish && w == last_w && h == last_h) return color_buf;
    last_w = w;
    last_h = h;
    cur_state = &list_state[list_shown];
    Raster ra = {w, h, k, margin, polys[mid ? LIST_MID : list_shown], poly_count[mid ? LIST_MID : list_shown]};
    for (int i = 0; i < 32; i++) {
        u32 c = cur_state->toon[i];
        toon8[i] = ((c & 31) * 255 / 31) | (((c >> 5) & 31) * 255 / 31) << 8 | (((c >> 10) & 31) * 255 / 31) << 16;
    }
    for (int i = 0; i < 8; i++) {
        u32 c = cur_state->edge[i];
        edge8[i] = ((c & 31) * 255 / 31) | (((c >> 5) & 31) * 255 / 31) << 8 | (((c >> 10) & 31) * 255 / 31) << 16;
    }
    tex_cache_check();
    prepare_polys(&ra);
    gfx_parallel(clear_band, &ra);
    next_strip = 0;
    gfx_parallel(raster_band, &ra);
    if (cur_state->disp3d & 0x20) gfx_parallel(edge_band, &ra);
    return color_buf;
}

u32 nds_swaps;

/* appele a chaque VBlank par le fil du jeu : si le jeu a demande l'echange, sa liste part au dessin */
void gx_begin_frame(void) {
    if (!swap_pending) return;
    swap_pending = 0;
    nds_swaps++;
    ListState *st = &list_state[build];
    for (int i = 0; i < 4; i++) {
        u8 *src = vram_tex_slot(i);
        st->has_tex[i] = src != 0;
        if (src) nds_copy(st->tex[i], src, KB(128));
    }
    for (int i = 0; i < 6; i++) {
        u8 *src = vram_texpal_slot(i);
        st->has_pal[i] = src != 0;
        if (src) nds_copy(st->pal[i], src, KB(16));
    }
    st->disp3d = REG16(IO + 0x60);
    st->clear = REG32(IO + 0x350);
    st->cdepth = REG16(IO + 0x354) & 0x7FFF;
    st->aref = REG8(IO + 0x340) & 31;
    st->flags = swap_flags;
    nds_copy(st->toon, DS(IO + 0x380), 64);
    nds_copy(st->edge, DS(IO + 0x330), 16);
    st->vx1 = vp_x1;
    st->vy1 = vp_y1;
    st->vx2 = vp_x2;
    st->vy2 = vp_y2;
    REG16(IO + 0x604) = (u16)poly_count[build];
    while (__sync_lock_test_and_set(&list_lock, 1)) os_yield();
    int t = list_ready;
    list_ready = build;
    build = t;
    list_fresh = 1;
    __sync_lock_release(&list_lock);
    poly_count[build] = 0;
}
