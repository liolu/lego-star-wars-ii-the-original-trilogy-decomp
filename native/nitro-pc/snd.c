/* Son : le travail du firmware ARM7 (sequenceur SSEQ, banques SBNK, ondes SWAV, flux PCM) et le mixeur. */
#include "nitro.h"

#define RATE 32768
#define CHANNELS 16
#define PLAYERS 16
#define TRACKS 32
#define UPDATE_SAMPLES 171 /* le firmware met son sequenceur a jour toutes les 5,2 ms */

volatile u32 *snd_shared(void);

static volatile long lock_word;
static void lock(void) { while (__sync_lock_test_and_set(&lock_word, 1)) os_yield(); }
static void unlock(void) { __sync_lock_release(&lock_word); }

/* ------------------------------------------------------------------ tables */
static float cb_table[1024];     /* attenuation en 1/10 dB -> gain */
static float pitch_table[768];   /* demi-ton / 64 -> rapport */
static s16 vel_table[128];       /* volume 0..127 -> 1/10 dB */

static double ln(double x) {
#if defined(__i386__) || defined(_WIN32) /* sans libm sous Windows */
    double r;
    __asm__("fldln2\n fxch\n fyl2x" : "=t"(r) : "0"(x) : "st(1)");
    return r;
#else
    return __builtin_log(x);
#endif
}

static void tables_init(void) {
    double step = 1.0 / 1.00115794542; /* 10^(-0.1/20) */
    double g = 1.0;
    for (int i = 0; i < 1024; i++) { cb_table[i] = (float)g; g *= step; }
    double p = 1.0, up = 1.00090293;   /* 2^(1/768) */
    for (int i = 0; i < 768; i++) { pitch_table[i] = (float)p; p *= up; }
    vel_table[0] = -723;
    for (int v = 1; v < 128; v++) {
        double db = 400.0 * ln(v / 127.0) / 2.302585093;
        vel_table[v] = (s16)(db < -723 ? -723 : db);
    }
}

static inline float gain(int level) {
    if (level >= 0) return 1.0f;
    if (level <= -1023) return 0;
    return cb_table[-level];
}

static float pitch_ratio(int fine) { /* fine : 1/64 de demi-ton */
    int oct = 0;
    while (fine < 0) { fine += 768; oct--; }
    while (fine >= 768) { fine -= 768; oct++; }
    float r = pitch_table[fine];
    while (oct > 0) { r *= 2; oct--; }
    while (oct < 0) { r *= 0.5f; oct++; }
    return r;
}

static const s16 ima_step[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const s8 ima_index[8] = {-1,-1,-1,-1,2,4,6,8};

/* ------------------------------------------------------------------ canaux */
typedef struct {
    u8 active, kind; /* kind 0 onde, 1 carre, 2 bruit */
    u8 fmt, loop, locked, seq, player, track, note, velocity, prio, releasing, started;
    const u8 *data;
    u32 loop_start, loop_len, length; /* en octets */
    u32 pos_frac;                     /* position en octets (ou echantillons), fixe */
    u32 pos;                          /* echantillon */
    u32 nib;                          /* ADPCM : quartet suivant */
    s32 pred, index, loop_pred, loop_index, loop_saved, cur;
    u32 base_timer, timer;
    float step, gain_l, gain_r;
    s32 env;                          /* enveloppe, 0 = plein, -92544 = silence */
    u8 attack;
    u16 decay, release;
    s32 sustain;
    u8 phase;
    s32 ticks;                        /* duree restante de la note */
    int vol_raw, shift_raw, pan_raw;
    u32 noise;
    u32 duty;
} Chan;
static Chan ch[CHANNELS];

static void chan_gain(Chan *c, float g, int pan) {
    if (pan < 0) pan = 0;
    if (pan > 127) pan = 127;
    c->gain_l = g * (127 - pan) / 127.0f;
    c->gain_r = g * pan / 127.0f;
}

static inline s32 chan_sample(Chan *c) {
    switch (c->kind) {
    case 1: { /* carre */
        u32 ph = (c->pos >> 13) & 7; /* 8 pas */
        return ph <= c->duty ? 32767 : -32768;
    }
    case 2: { /* bruit */
        if (c->pos_frac >= 0x10000) {
            c->noise = (c->noise & 1) ? (c->noise >> 1) ^ 0x6000 : c->noise >> 1;
        }
        return (c->noise & 1) ? -32768 : 32767;
    }
    }
    return c->cur;
}

static void chan_advance_wave(Chan *c, u32 n) {
    while (n--) {
        switch (c->fmt) {
        case 0:
            c->pos++;
            if (c->pos >= c->length) {
                if (!c->loop) { c->active = 0; return; }
                c->pos = c->loop_start;
            }
            c->cur = (s8)c->data[c->pos] << 8;
            break;
        case 1:
            c->pos += 2;
            if (c->pos >= c->length) {
                if (!c->loop) { c->active = 0; return; }
                c->pos = c->loop_start;
            }
            c->cur = *(s16 *)(c->data + c->pos);
            break;
        default: {
            /* IMA ADPCM : 4 octets d'en-tete, puis deux quartets par octet */
            if (c->loop && !c->loop_saved && c->nib >= c->loop_start * 2) {
                c->loop_saved = 1;
                c->loop_pred = c->pred;
                c->loop_index = c->index;
            }
            if (c->nib >= c->length * 2) {
                if (!c->loop) { c->active = 0; return; }
                c->nib = c->loop_start * 2;
                c->pred = c->loop_pred;
                c->index = c->loop_index;
            }
            u32 v = (c->data[c->nib >> 1] >> ((c->nib & 1) * 4)) & 15;
            c->nib++;
            s32 st = ima_step[c->index];
            s32 diff = st >> 3;
            if (v & 1) diff += st >> 2;
            if (v & 2) diff += st >> 1;
            if (v & 4) diff += st;
            if (v & 8) { c->pred -= diff; if (c->pred < -32768) c->pred = -32768; }
            else { c->pred += diff; if (c->pred > 32767) c->pred = 32767; }
            c->index += ima_index[v & 7];
            if (c->index < 0) c->index = 0;
            if (c->index > 88) c->index = 88;
            c->cur = c->pred;
            break;
        }
        }
    }
}

static void chan_start_wave(Chan *c, const u8 *data, u32 fmt, u32 loop, u32 loop_start, u32 loop_len) {
    c->active = 1;
    c->kind = 0;
    c->data = data;
    c->fmt = (u8)fmt;
    c->loop = loop != 0 && loop != 2;
    c->loop_start = loop_start;
    c->loop_len = loop_len;
    c->length = loop_start + loop_len;
    c->pos = 0;
    c->pos_frac = 0;
    c->nib = 8;
    c->loop_saved = 0;
    if (fmt == 2) {
        c->pred = *(s16 *)data;
        c->index = data[2] & 127;
        if (c->index > 88) c->index = 88;
        c->cur = c->pred;
    } else if (fmt == 1) c->cur = *(s16 *)data;
    else c->cur = (s8)data[0] << 8;
}

/* ------------------------------------------------------------------ sequenceur */
typedef struct {
    u8 active, player, index, mute;
    const u8 *base, *pc, *call[3];
    const u8 *loop_pc[3];
    u8 loop_n[3], call_sp;
    u16 prog;
    s32 wait;
    u8 pan_raw;
    u8 vol, expr, prio, poly_mono, tie, bend_range, attack, decay, sustain, release;
    s8 pan_user, transpose, pitch_bend;
    s16 fader;
    u8 port_on, port_key, port_time;
    u8 mod_type, mod_speed, mod_depth, mod_range;
    u16 mod_delay;
    u16 mod_counter, mod_phase;
    s16 sweep;
    u16 alloc_mask;
    s32 cond;
    u8 prog_set;
} Track;

typedef struct {
    u8 active, paused, prepared, prio;
    u8 tracks[16];
    const u8 *bank;
    u16 tempo, tempo_ratio, tempo_acc;
    s16 fader;
    u8 master_vol;
    u32 ticks;
} Player;

static Player players[PLAYERS];
static Track tracks[TRACKS];
static s16 global_var[16];

static s16 *var_ptr(Player *pl, u32 n, s16 *fallback) {
    volatile u32 *sh = snd_shared();
    if (!sh) return fallback;
    if (n >= 16) return (s16 *)((volatile u8 *)sh + 0x20 + 16 * 0x24) + (n - 16);
    int p = (int)(pl - players);
    return (s16 *)((volatile u8 *)sh + 0x20 + p * 0x24) + n;
}

static u32 varlen(const u8 **pc) {
    u32 v = 0;
    u8 b;
    do { b = *(*pc)++; v = (v << 7) | (b & 0x7F); } while (b & 0x80);
    return v;
}

static const u8 attack_low[19] = {0, 1, 5, 14, 26, 38, 51, 63, 73, 84, 92, 100, 109, 116, 123, 127, 132, 137, 143};

static u32 attack_rate(u32 v) { return v < 109 ? 255 - v : attack_low[127 - (v > 127 ? 127 : v)]; }
static u32 fall_rate(u32 v) {
    if (v == 127) return 0xFFFF;
    if (v == 126) return 0x3C00;
    if (v < 50) return v * 2 + 1;
    return 0x1E00 / (126 - v);
}

/* bank : banque en memoire DS ; ses archives d'ondes y sont rangees par adresse DS */
static const u8 *arc_wave(const u8 *bank, u32 arc_slot, u32 wave) {
    u32 arc_addr = *(u32 *)(bank + 0x18 + arc_slot * 8);
    if (!arc_addr) return 0;
    const u8 *arc = DS(arc_addr);
    if (wave >= *(u32 *)(arc + 0x38)) return 0;
    u32 v = *(u32 *)(arc + 0x3C + wave * 4);
    if (!v) return 0;
    return v < 0x02000000u ? arc + v : DS(v);
}

/* trouve l'instrument et son ton pour une note */
typedef struct { u32 kind, a, b; u8 note, attack, decay, sustain, release, pan; } Tone;

static int find_tone(const u8 *bank, u32 prog, u32 key, Tone *t) {
    if (!bank) return 0;
    if (prog >= *(u32 *)(bank + 0x38)) return 0;
    u32 entry = *(u32 *)(bank + 0x3C + prog * 4);
    u32 type = entry & 0xFF, off = entry >> 8;
    if (!type) return 0;
    const u8 *d = bank + off;
    if (type >= 16) {
        if (type == 16) {
            u32 lo = d[0], hi = d[1];
            if (key < lo || key > hi) return 0;
            d += 2 + (key - lo) * 12;
        } else {
            int i = 0;
            while (i < 8 && d[i] && key > d[i]) i++;
            if (i == 8 || !d[i]) return 0;
            d += 8 + i * 12;
        }
        type = *(u16 *)d;
        d += 2;
    }
    t->kind = type;
    t->a = *(u16 *)d;
    t->b = *(u16 *)(d + 2);
    t->note = d[4];
    t->attack = d[5];
    t->decay = d[6];
    t->sustain = d[7];
    t->release = d[8];
    t->pan = d[9];
    return 1;
}

static Chan *chan_alloc(Track *tr, int prio) {
    u32 mask = tr->alloc_mask;
    Chan *best = 0;
    for (int i = CHANNELS - 1; i >= 0; i--) {
        if (!(mask & (1u << i)) || ch[i].locked) continue;
        Chan *c = &ch[i];
        if (!c->active) return c;
        if (!best || c->prio < best->prio || (c->prio == best->prio && c->env < best->env)) best = c;
    }
    if (best && best->prio <= prio) return best;
    return 0;
}

static void chan_update_voice(Chan *c) {
    /* volume, panoramique et hauteur : recalcules a chaque pas */
    if (c->player >= PLAYERS) return;
    Player *pl = &players[c->player];
    Track *tr = &tracks[c->track];
    int level = vel_table[c->velocity] + vel_table[tr->vol] + vel_table[tr->expr] + vel_table[pl->master_vol] + tr->fader + pl->fader + c->env / 128;
    /* modulation */
    int mod = 0;
    if (tr->mod_depth && tr->mod_counter >= tr->mod_delay) {
        int ph = tr->mod_phase >> 8; /* 0..255 */
        int tri = ph < 64 ? ph : ph < 192 ? 128 - ph : ph - 256;
        mod = tri * tr->mod_depth * tr->mod_range / 64;
    }
    int pitch = tr->pitch_bend * tr->bend_range + (tr->mod_type == 0 ? mod : 0) + tr->sweep;
    float ratio = pitch_ratio(pitch);
    c->step = (float)(16756991.0 / (double)c->base_timer) * ratio / RATE;
    int pan = (int)tr->pan_user + (tr->mod_type == 2 ? mod / 64 : 0) + c->pan_raw;
    chan_gain(c, gain(level + (tr->mod_type == 1 ? mod / 64 : 0)), pan);
}

static void note_on(Player *pl, Track *tr, u32 key, u32 vel, s32 length) {
    Tone t;
    int k = (int)key + tr->transpose;
    if (k < 0) k = 0;
    if (k > 127) k = 127;
    if (!find_tone(pl->bank, tr->prog, (u32)k, &t)) return;
    if (tr->tie) {
        for (int i = 0; i < CHANNELS; i++) {
            Chan *c = &ch[i];
            if (c->active && c->seq && c->player == tr->player && c->track == tr->index && !c->releasing) {
                c->velocity = (u8)vel;
                c->note = (u8)k;
                return;
            }
        }
    }
    Chan *c = chan_alloc(tr, tr->prio);
    if (!c) return;
    int fine = (k - t.note) * 64;
    float r = pitch_ratio(fine);
    u32 timer;
    if (t.kind == 1) {
        const u8 *w = arc_wave(pl->bank, t.b, t.a);
        if (!w) return;
        u32 fmt = w[0], loop = w[1], tim = *(u16 *)(w + 4), ls = *(u16 *)(w + 6), ll = *(u32 *)(w + 8);
        chan_start_wave(c, w + 12, fmt, loop, ls * 4, ll * 4);
        timer = tim;
    } else if (t.kind == 2) { /* carre */
        c->active = 1; c->kind = 1; c->duty = t.a; c->pos = 0;
        timer = 8006;
        c->loop = 1;
    } else if (t.kind == 3) { /* bruit */
        c->active = 1; c->kind = 2; c->noise = 0x7FFF; c->pos = 0;
        timer = 8006;
        c->loop = 1;
    } else return;
    c->base_timer = (u32)(timer / r);
    if (c->base_timer < 1) c->base_timer = 1;
    c->seq = 1;
    c->player = (u8)(pl - players);
    c->track = tr->index;
    c->note = (u8)k;
    c->velocity = (u8)vel;
    c->prio = tr->prio;
    c->pos_frac = 0;
    c->ticks = length;
    c->releasing = 0;
    c->env = -92544;
    c->phase = 0;
    c->attack = (u8)attack_rate(tr->attack != 0xFF ? tr->attack : t.attack);
    c->decay = (u16)fall_rate(tr->decay != 0xFF ? tr->decay : t.decay);
    {
        int s = tr->sustain != 0xFF ? tr->sustain : t.sustain;
        c->sustain = (s32)vel_table[s] * 128;
    }
    c->release = (u16)fall_rate(tr->release != 0xFF ? tr->release : t.release);
    c->pan_raw = t.pan;
    chan_update_voice(c);
}

static void track_stop(Track *tr) {
    tr->active = 0;
    for (int i = 0; i < CHANNELS; i++) {
        Chan *c = &ch[i];
        if (c->active && c->seq && c->player == tr->player && c->track == tr->index) c->releasing = 1;
    }
}

static void track_init(Track *tr, Player *pl, int index_in_player, const u8 *pc) {
    u8 idx = tr->index;
    nds_zero(tr, sizeof(*tr));
    tr->index = idx;
    tr->active = 1;
    tr->player = (u8)(pl - players);
    tr->pc = pc;
    tr->vol = 127;
    tr->expr = 127;
    tr->prio = pl->prio;
    tr->bend_range = 2;
    tr->pan_raw = 64;
    tr->attack = tr->decay = tr->sustain = tr->release = 0xFF;
    tr->mod_range = 1;
    tr->mod_speed = 16;
    tr->alloc_mask = 0xFFFF;
    (void)index_in_player;
}

static void track_tick(Player *pl, Track *tr) {
    if (tr->wait > 0) {
        tr->wait--;
        return;
    }
    int guard = 4096;
    while (tr->active && tr->wait <= 0 && guard-- > 0) {
        const u8 *pc = tr->pc;
        u32 op = *pc++;
        int cond_ok = 1;
        if (op == 0xA2) { cond_ok = tr->cond; op = *pc++; }
        if (op < 0x80) {
            u32 vel = *pc++;
            s32 dur = varlen(&pc);
            tr->pc = pc;
            if (cond_ok && !tr->mute) note_on(pl, tr, op, vel, dur);
            if (tr->poly_mono) tr->wait = dur;
            continue;
        }
        s32 val16 = 0;
        switch (op) {
        case 0x80: tr->wait = varlen(&pc); tr->pc = pc; continue;
        case 0x81: {
            u32 v = varlen(&pc);
            if (cond_ok) tr->prog = (u16)v;
            break;
        }
        case 0x93: {
            u32 n = pc[0];
            u32 off = pc[1] | (pc[2] << 8) | (pc[3] << 16);
            pc += 4;
            if (cond_ok && n < 16) {
                for (int i = 0; i < TRACKS; i++) {
                    if (!tracks[i].active) {
                        track_init(&tracks[i], pl, n, tr->base + off);
                        tracks[i].base = tr->base;
                        pl->tracks[n] = (u8)i;
                        break;
                    }
                }
            }
            break;
        }
        case 0x94: {
            u32 off = pc[0] | (pc[1] << 8) | (pc[2] << 16);
            pc += 3;
            if (cond_ok) pc = tr->base + off;
            break;
        }
        case 0x95: {
            u32 off = pc[0] | (pc[1] << 8) | (pc[2] << 16);
            pc += 3;
            if (cond_ok && tr->call_sp < 3) { tr->call[tr->call_sp++] = pc; pc = tr->base + off; }
            break;
        }
        case 0xC0: if (cond_ok) tr->pan_user = (s8)(pc[0] - 64); pc++; break;
        case 0xC1: if (cond_ok) tr->vol = pc[0] & 127; pc++; break;
        case 0xC2: if (cond_ok) pl->master_vol = pc[0] & 127; pc++; break;
        case 0xC3: if (cond_ok) tr->transpose = (s8)pc[0]; pc++; break;
        case 0xC4: if (cond_ok) tr->pitch_bend = (s8)pc[0]; pc++; break;
        case 0xC5: if (cond_ok) tr->bend_range = pc[0]; pc++; break;
        case 0xC6: if (cond_ok) tr->prio = pc[0]; pc++; break;
        case 0xC7: if (cond_ok) tr->poly_mono = pc[0]; pc++; break;
        case 0xC8: if (cond_ok) tr->tie = pc[0]; pc++; break;
        case 0xC9: if (cond_ok) { tr->port_key = pc[0]; tr->port_on = 1; } pc++; break;
        case 0xCA: if (cond_ok) tr->mod_depth = pc[0]; pc++; break;
        case 0xCB: if (cond_ok) tr->mod_speed = pc[0]; pc++; break;
        case 0xCC: if (cond_ok) tr->mod_type = pc[0]; pc++; break;
        case 0xCD: if (cond_ok) tr->mod_range = pc[0]; pc++; break;
        case 0xCE: if (cond_ok) tr->port_on = pc[0]; pc++; break;
        case 0xCF: if (cond_ok) tr->port_time = pc[0]; pc++; break;
        case 0xD0: if (cond_ok) tr->attack = pc[0]; pc++; break;
        case 0xD1: if (cond_ok) tr->decay = pc[0]; pc++; break;
        case 0xD2: if (cond_ok) tr->sustain = pc[0]; pc++; break;
        case 0xD3: if (cond_ok) tr->release = pc[0]; pc++; break;
        case 0xD4:
            if (tr->call_sp < 3) {
                tr->loop_pc[tr->call_sp] = pc + 1;
                tr->loop_n[tr->call_sp] = pc[0];
                tr->call[tr->call_sp] = 0;
                tr->call_sp++;
            }
            pc++;
            break;
        case 0xD5: if (cond_ok) tr->expr = pc[0] & 127; pc++; break;
        case 0xD6: pc++; break;
        case 0xE0: val16 = *(s16 *)pc; pc += 2; if (cond_ok) tr->mod_delay = (u16)val16; break;
        case 0xE1: val16 = *(u16 *)pc; pc += 2; if (cond_ok) pl->tempo = (u16)val16; break;
        case 0xE3: val16 = *(s16 *)pc; pc += 2; if (cond_ok) tr->sweep = (s16)val16; break;
        case 0xFC:
            if (tr->call_sp) {
                int sp = tr->call_sp - 1;
                if (tr->loop_n[sp] == 0 || --tr->loop_n[sp]) pc = tr->loop_pc[sp];
                else tr->call_sp = (u8)sp;
            }
            break;
        case 0xFD:
            if (tr->call_sp) {
                const u8 *back = tr->call[tr->call_sp - 1];
                tr->call_sp--;
                if (back) pc = back;
            }
            break;
        case 0xFE: pc += 2; break;
        case 0xFF: track_stop(tr); tr->pc = pc; return;
        default:
            if (op >= 0xB0 && op <= 0xBD) {
                u32 n = *pc++;
                s16 v = *(s16 *)pc;
                pc += 2;
                static s16 dummy;
                s16 *var = var_ptr(pl, n, &dummy);
                if (cond_ok) {
                    switch (op) {
                    case 0xB0: *var = v; break;
                    case 0xB1: *var += v; break;
                    case 0xB2: *var -= v; break;
                    case 0xB3: *var *= v; break;
                    case 0xB4: if (v) *var /= v; break;
                    case 0xB5: *var = v >= 0 ? *var << v : *var >> -v; break;
                    case 0xB6: break;
                    case 0xB8: tr->cond = *var == v; break;
                    case 0xB9: tr->cond = *var >= v; break;
                    case 0xBA: tr->cond = *var > v; break;
                    case 0xBB: tr->cond = *var <= v; break;
                    case 0xBC: tr->cond = *var < v; break;
                    case 0xBD: tr->cond = *var != v; break;
                    }
                }
            } else if (op == 0xA0) {
                /* valeur aleatoire pour la commande suivante : on saute ses octets */
                u32 sub = *pc++;
                if (sub < 0x80) { pc += 1; pc += 4; }
                else if (sub >= 0xB0 && sub <= 0xBD) { pc += 1; pc += 4; }
                else if (sub >= 0xE0 && sub < 0xF0) pc += 4;
                else pc += 4;
            } else if (op == 0xA1) {
                u32 sub = *pc++;
                pc += sub >= 0xB0 && sub <= 0xBD ? 2 : 1;
            } else {
                /* commande inconnue : piste arretee plutot que du bruit */
                track_stop(tr);
                tr->pc = pc;
                return;
            }
            break;
        }
        tr->pc = pc;
    }
}

static void player_stop(Player *pl) {
    for (int i = 0; i < 16; i++) {
        if (pl->tracks[i] != 0xFF) { track_stop(&tracks[pl->tracks[i]]); pl->tracks[i] = 0xFF; }
    }
    pl->active = 0;
    pl->prepared = 0;
    volatile u32 *sh = snd_shared();
    if (sh) sh[1] &= ~(1u << (pl - players));
}

static void player_start(Player *pl, const u8 *base, u32 off, const u8 *bank) {
    player_stop(pl);
    pl->bank = bank;
    pl->tempo = 120;
    pl->tempo_ratio = 256;
    pl->tempo_acc = 240;
    pl->master_vol = 127;
    pl->active = 1;
    pl->paused = 1; /* attend START_PREPARED, ou demarre tout de suite */
    pl->prepared = 1;
    for (int i = 0; i < TRACKS; i++) {
        if (!tracks[i].active) {
            tracks[i].index = (u8)i;
            track_init(&tracks[i], pl, 0, base + off);
            tracks[i].base = base;
            pl->tracks[0] = (u8)i;
            break;
        }
    }
    volatile u32 *sh = snd_shared();
    if (sh) sh[1] |= 1u << (pl - players);
}

static void seq_update(void) {
    for (int p = 0; p < PLAYERS; p++) {
        Player *pl = &players[p];
        if (!pl->active || pl->paused) continue;
        u32 add = (u32)pl->tempo * pl->tempo_ratio / 256;
        pl->tempo_acc += add;
        while (pl->tempo_acc >= 240) {
            pl->tempo_acc -= 240;
            int any = 0;
            for (int t = 0; t < 16; t++) {
                if (pl->tracks[t] == 0xFF) continue;
                Track *tr = &tracks[pl->tracks[t]];
                if (!tr->active) continue;
                track_tick(pl, tr);
                if (tr->active) any = 1;
            }
            pl->ticks++;
            volatile u32 *sh = snd_shared();
            if (sh) *(u32 *)((volatile u8 *)sh + 0x20 + p * 0x24 + 0x20) = pl->ticks;
            if (!any) { player_stop(pl); break; }
            /* notes de duree limitee */
            for (int i = 0; i < CHANNELS; i++) {
                Chan *c = &ch[i];
                if (c->active && c->seq && c->player == p && !c->releasing && c->ticks > 0) {
                    if (--c->ticks == 0) c->releasing = 1;
                }
            }
        }
        /* modulation */
        for (int t = 0; t < 16; t++) {
            if (pl->tracks[t] == 0xFF) continue;
            Track *tr = &tracks[pl->tracks[t]];
            if (tr->mod_depth) {
                if (tr->mod_counter < tr->mod_delay) tr->mod_counter++;
                else tr->mod_phase = (u16)(tr->mod_phase + tr->mod_speed * 64);
            }
        }
    }
    /* enveloppes */
    for (int i = 0; i < CHANNELS; i++) {
        Chan *c = &ch[i];
        if (!c->active || !c->seq) continue;
        if (c->releasing) {
            c->env -= c->release;
            if (c->env <= -92544) { c->active = 0; continue; }
        } else if (c->phase == 0) {
            c->env = (s32)(((s64)c->env * c->attack) >> 8);
            if (c->env >= -128) { c->env = 0; c->phase = 1; }
        } else if (c->phase == 1) {
            c->env -= c->decay;
            if (c->env <= c->sustain) { c->env = c->sustain; c->phase = 2; }
        }
        chan_update_voice(c);
    }
}

/* ------------------------------------------------------------------ alarmes (flux) */
typedef struct { u8 active, id; u32 next, period; } Alarm;
static Alarm alarms[8];
static u32 sample_clock;
static volatile u32 alarm_q[32], alarm_w, alarm_r;

#define TICKS_TO_SAMPLES(t) ((u32)(((u64)(t) * RATE) / 523655u))

static void alarms_step(void) {
    for (int i = 0; i < 8; i++) {
        Alarm *a = &alarms[i];
        if (!a->active) continue;
        if ((s32)(sample_clock - a->next) >= 0) {
            if (a->period) a->next += a->period;
            else a->active = 0;
            alarm_q[alarm_w & 31] = i | ((u32)a->id << 8);
            alarm_w++;
        }
    }
}

/* ------------------------------------------------------------------ commandes */
static int pending_start[CHANNELS];

void snd_command(u32 id, u32 *arg) {
    lock();
    switch (id) {
    case 0: /* START_SEQ */
    case 2: /* PREPARE_SEQ */
        if (arg[0] < PLAYERS) {
            /* sequence et banque : adresses DS ; banque 0 = aucune */
            player_start(&players[arg[0]], DS(arg[1]), arg[2], arg[3] ? DS(arg[3]) : 0);
            if (id == 0) players[arg[0]].paused = 0;
        }
        break;
    case 1: if (arg[0] < PLAYERS) player_stop(&players[arg[0]]); break;
    case 3: if (arg[0] < PLAYERS) players[arg[0]].paused = 0; break;
    case 4: if (arg[0] < PLAYERS) players[arg[0]].paused = arg[1] != 0; break;
    case 6: { /* parametre de lecteur : decalage dans la structure du firmware */
        if (arg[0] >= PLAYERS) break;
        Player *pl = &players[arg[0]];
        if (arg[1] == 6) pl->fader = (s16)arg[2];
        else if (arg[1] == 0x1A) pl->tempo_ratio = (u16)arg[2];
        else if (arg[1] == 4) pl->prio = (u8)arg[2];
        break;
    }
    case 7: { /* parametre de piste */
        u32 p = arg[0] & 0xFFFF;
        if (p >= PLAYERS) break;
        for (int t = 0; t < 16; t++) {
            if (!(arg[1] & (1u << t)) || players[p].tracks[t] == 0xFF) continue;
            Track *tr = &tracks[players[p].tracks[t]];
            if (arg[2] == 0xA) tr->fader = (s16)arg[3];
            else if (arg[2] == 9) tr->pan_user = (s8)arg[3];
        }
        break;
    }
    case 8: { /* silence de pistes */
        u32 p = arg[0];
        if (p >= PLAYERS) break;
        for (int t = 0; t < 16; t++)
            if ((arg[1] & (1u << t)) && players[p].tracks[t] != 0xFF) tracks[players[p].tracks[t]].mute = arg[2] != 0;
        break;
    }
    case 9: { /* canaux permis */
        u32 p = arg[0];
        if (p >= PLAYERS) break;
        for (int t = 0; t < 16; t++)
            if ((arg[1] & (1u << t)) && players[p].tracks[t] != 0xFF) tracks[players[p].tracks[t]].alloc_mask = (u16)arg[2];
        break;
    }
    case 12: { /* START_TIMER */
        for (int i = 0; i < CHANNELS; i++)
            if ((arg[0] & (1u << i)) && pending_start[i]) { ch[i].started = 1; pending_start[i] = 0; }
        for (int i = 0; i < 8; i++)
            if (arg[2] & (1u << i)) alarms[i].active = alarms[i].period || alarms[i].next ? 1 : 0;
        break;
    }
    case 13: { /* STOP_TIMER */
        for (int i = 0; i < CHANNELS; i++)
            if (arg[0] & (1u << i)) { ch[i].active = 0; ch[i].started = 0; }
        for (int i = 0; i < 8; i++)
            if (arg[2] & (1u << i)) alarms[i].active = 0;
        break;
    }
    case 14: { /* SETUP_CHANNEL_PCM */
        u32 n = arg[0] & 0xFFFF;
        if (n >= CHANNELS) break;
        Chan *c = &ch[n];
        u32 timer = arg[0] >> 16;
        u32 fmt = (arg[3] >> 24) & 3, loop = (arg[3] >> 26) & 3;
        u32 ls = (arg[3] & 0xFFFF) * 4, ll = (arg[2] & 0x3FFFFF) * 4;
        chan_start_wave(c, DS(arg[1]), fmt, loop, ls, ll);
        c->seq = 0;
        c->player = 0xFF;
        c->started = 0;
        pending_start[n] = 1;
        c->base_timer = timer ? timer : 1;
        c->step = (float)(16756991.0 / c->base_timer) / RATE;
        {
            u32 vol = arg[2] >> 24, shift = (arg[2] >> 22) & 3, pan = (arg[3] >> 16) & 127;
            static const u8 sh_tab[4] = {0, 1, 2, 4};
            c->vol_raw = vol & 127;
            c->shift_raw = sh_tab[shift];
            c->pan_raw = pan;
            chan_gain(c, (float)c->vol_raw / 127.0f / (float)(1 << c->shift_raw), pan);
        }
        break;
    }
    case 18: { /* SETUP_ALARM */
        u32 n = arg[0];
        if (n >= 8) break;
        alarms[n].id = (u8)arg[3];
        alarms[n].period = TICKS_TO_SAMPLES(arg[2]);
        alarms[n].next = sample_clock + TICKS_TO_SAMPLES(arg[1]);
        alarms[n].active = 0;
        break;
    }
    case 19: /* minuterie d'un canal */
        for (int i = 0; i < CHANNELS; i++)
            if ((arg[0] & (1u << i)) && arg[1]) { ch[i].base_timer = arg[1]; ch[i].step = (float)(16756991.0 / arg[1]) / RATE; }
        break;
    case 20: { /* volume de canaux */
        static const u8 sh_tab[4] = {0, 1, 2, 4};
        for (int i = 0; i < CHANNELS; i++) {
            if (!(arg[0] & (1u << i))) continue;
            ch[i].vol_raw = arg[1] & 127;
            ch[i].shift_raw = sh_tab[arg[2] & 3];
            if (!ch[i].seq) chan_gain(&ch[i], (float)ch[i].vol_raw / 127.0f / (float)(1 << ch[i].shift_raw), ch[i].pan_raw);
        }
        break;
    }
    case 21: /* panoramique */
        for (int i = 0; i < CHANNELS; i++) {
            if (!(arg[0] & (1u << i))) continue;
            ch[i].pan_raw = arg[1] & 127;
            if (!ch[i].seq) chan_gain(&ch[i], (float)ch[i].vol_raw / 127.0f / (float)(1 << ch[i].shift_raw), ch[i].pan_raw);
        }
        break;
    case 26: for (int i = 0; i < CHANNELS; i++) if (arg[0] & (1u << i)) { ch[i].locked = 1; if (ch[i].seq) ch[i].active = 0; } break;
    case 27: for (int i = 0; i < CHANNELS; i++) if (arg[0] & (1u << i)) ch[i].locked = 0; break;
    default: break;
    }
    unlock();
}

/* appelee par le fil du jeu a chaque image : remet les alarmes arrivees au SDK */
void io_fifo_push(u32 word);
void snd_frame(void) {
    while (alarm_r != alarm_w) {
        io_fifo_push(7u | (alarm_q[alarm_r & 31] << 6));
        alarm_r++;
        io_raise(1u << 18);
    }
}

/* ------------------------------------------------------------------ mixeur */
static float master = 0.6f;
static int muted;

/* Volume choisi dans le launcher (paquet, mot 15), en pour cent du volume d'origine. */
void snd_set_volume(u32 pct) {
    if (pct > 200) pct = 200;
    if (!muted) master = 0.6f * (float)pct / 100.0f;
}

static void mix_ds(s16 *out, u32 frames);

static void mix(s16 *out, u32 frames) {
    mix_ds(out, frames);
    if (nds_cb.audio) nds_mods_audio(out, frames);
}

static void mix_ds(s16 *out, u32 frames) {
    static u32 until_update;
    for (u32 f = 0; f < frames; f++) {
        if (until_update == 0) {
            lock();
            seq_update();
            unlock();
            until_update = UPDATE_SAMPLES;
        }
        until_update--;
        sample_clock++;
        alarms_step();
        float l = 0, r = 0;
        for (int i = 0; i < CHANNELS; i++) {
            Chan *c = &ch[i];
            if (!c->active || (!c->seq && !c->started)) continue;
            s32 sample;
            if (c->kind == 0) {
                sample = c->cur;
                u32 step = (u32)(c->step * 65536.0f);
                c->pos_frac += step;
                u32 adv = c->pos_frac >> 16;
                c->pos_frac &= 0xFFFF;
                if (adv) chan_advance_wave(c, adv);
            } else if (c->kind == 1) {
                u32 step = (u32)(c->step * 65536.0f);
                c->pos += step;
                sample = chan_sample(c);
            } else {
                u32 step = (u32)(c->step * 65536.0f);
                c->pos_frac += step;
                sample = chan_sample(c);
                if (c->pos_frac >= 0x10000) c->pos_frac &= 0xFFFF;
            }
            l += sample * c->gain_l;
            r += sample * c->gain_r;
        }
        l *= master;
        r *= master;
        if (l > 32767) l = 32767;
        if (l < -32768) l = -32768;
        if (r > 32767) r = 32767;
        if (r < -32768) r = -32768;
        out[f * 2] = (s16)l;
        out[f * 2 + 1] = (s16)r;
    }
}

#define BUF_FRAMES 384

static void audio_thread(void *arg) {
    static s16 idle_buf[BUF_FRAMES * 2];
    OsAudio *dev = 0;
    if (!nds_arg("--mute", 0)) {
        dev = os_audio_open(RATE, BUF_FRAMES);
        if (!dev) nds_log("pas de sortie son");
    }
    os_thread_set_high_priority();
    if (dev) {
        /* amorce : tous les tampons sont remplis avant que la sortie ne commence a consommer */
        for (u32 i = os_audio_depth(dev); i; i--) {
            s16 *b = os_audio_acquire(dev);
            mix(b, BUF_FRAMES);
            os_audio_submit(dev);
        }
    }
    const char *wav_path = 0;
    OsFile wav = OS_BAD_FILE;
    u32 wav_bytes = 0;
    if (nds_arg("--wav", &wav_path) && wav_path) {
        wav = os_file_create(wav_path);
        master = 0.6f;
    }
    for (;;) {
        if (wav != OS_BAD_FILE) {
            os_sleep(BUF_FRAMES * 1000 / RATE);
            mix(idle_buf, BUF_FRAMES);
            os_file_write(wav, idle_buf, sizeof(idle_buf));
            wav_bytes += sizeof(idle_buf);
            (void)wav_bytes;
        } else if (dev) {
            s16 *b = os_audio_acquire(dev);
            mix(b, BUF_FRAMES);
            os_audio_submit(dev);
        } else {
            /* sans sortie son (ou --mute), le sequenceur avance quand meme : les flux en dependent */
            os_sleep(BUF_FRAMES * 1000 / RATE);
            mix(idle_buf, BUF_FRAMES);
        }
    }
}

/* --fixed-clock : pas de fil audio, le son avance d'une image a chaque VBlank */
void snd_fixed_frame(void) {
    static s16 buf[(RATE / 60 + 1) * 2];
    mix(buf, RATE / 60);
}

void snd_start_output(void) {
    tables_init();
    for (int p = 0; p < PLAYERS; p++)
        for (int t = 0; t < 16; t++) players[p].tracks[t] = 0xFF;
    for (int t = 0; t < TRACKS; t++) tracks[t].index = (u8)t;
    if (nds_arg("--mute", 0)) { master = 0; muted = 1; }
    if (!nds_fixed_clock) os_thread_start(audio_thread, 0, 0x40000);
}
