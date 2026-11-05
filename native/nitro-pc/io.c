/* Registres materiels : la memoire DS 0x04000000 est de la vraie memoire, les effets passent ici via les exceptions. */
#include "nitro.h"

#define MS_FREQ 33513982ull /* horloge des timers */
#define LINES 263
#define VISIBLE 192

u32 nds_frame;

static s64 freq, t0;
static u32 if_state;
static u32 line_prev;
static int in_irq;

static struct { u32 reload, ctrl; u64 start; u64 next; } tm[4];

/* file de reponses du processeur ARM7 simule */
static u32 fifo_q[256];
static u32 fifo_r, fifo_w, fifo_top;

void io_fifo_push(u32 word) {
    if (fifo_w - fifo_r < 256) fifo_q[fifo_w++ & 255] = word;
}
static u32 fifo_count(void) { return fifo_w - fifo_r; }

static u32 clock_k; /* cycles par tic QPC, en virgule fixe 16 bits */

int nds_fixed_clock;
static u64 fixed_now;

/* Vitesse du jeu en pour cent (paquet du launcher, mot 14). Tant qu'elle n'a jamais change, l'horloge est exactement
 * celle d'avant ; ensuite le temps du jeu avance de (temps reel x vitesse), sans saut quand la vitesse change. */
static volatile u32 speed_pct = 100;
static int speed_used;
static s64 speed_last;
static u64 speed_ticks;

void io_set_speed(u32 pct) {
    if (pct < 10) pct = 10;
    if (pct > 1000) pct = 1000;
    speed_pct = pct;
}

static u64 cycles64(void) {
    if (nds_fixed_clock) return fixed_now += 1000;
    s64 now = os_time_now();
    u32 pct = speed_pct;
    if (!speed_used) {
        if (pct == 100) return ((u64)(now - t0) * clock_k) >> 16;
        speed_used = 1;
        speed_last = t0;
    }
    speed_ticks += (u64)(now - speed_last) * pct / 100;
    speed_last = now;
    return (speed_ticks * clock_k) >> 16;
}

u32 io_cycles(void) { return (u32)cycles64(); }

void io_raise(u32 bits) { if_state |= bits; }

/* acces direct : avec tout le jeu traduit, les registres ne passent jamais par les exceptions */
extern const int nds_has_c;
int nds_in_veh;
static int open_depth;
static void io_open(int on) {
    os_mem_protect(DS(IO), 0x1000, on ? OS_RW : OS_NOACCESS);
    os_mem_protect(DS(0x04100000u), 0x1000, on ? OS_RW : OS_NOACCESS);
}
/* le runtime touche aux registres sans passer par les exceptions */
#define OPEN() do { if (!nds_in_veh && open_depth++ == 0) io_open(1); } while (0)
#define CLOSE() do { if (!nds_in_veh && --open_depth == 0) io_open(0); } while (0)

int io_irq_ready(void) {
    if (in_irq || (nds_cpsr & 0x80) || !(REG32(IO + 0x208) & 1)) return 0;
    return (REG32(IO + 0x210) & if_state) != 0;
}

extern const u32 nds_old_sdk;

/* ------------------------------------------------------------------ DMA */
static u8 dma_on[4]; /* nds_old_sdk : une copie immediate part quand le canal s'active, pas a chaque ecriture de son controle */

static void dma_run(int i) {
    u32 base = IO + 0xB0 + i * 12;
    u32 src = REG32(base), dst = REG32(base + 4), cnt = REG32(base + 8);
    u32 mode = (cnt >> 27) & 7, n = cnt & 0x1FFFFF;
    int w32 = (cnt >> 26) & 1;
    int step = w32 ? 4 : 2;
    int sstep = ((cnt >> 23) & 3) == 0 ? step : ((cnt >> 23) & 3) == 1 ? -step : 0;
    int dstep = ((cnt >> 21) & 3) == 1 ? -step : ((cnt >> 21) & 3) == 2 ? 0 : step;
    if (!n) n = 0x200000;
#ifdef NDS_INSTRUMENT
    {
        extern u32 nds_watch, nds_pc, nds_r[6];
        u32 span = n * step;
        /* pas les copies vides de MI_StopDma (src = dst = 0) : le log lirait l'adresse 0 */
        if (src >= 0x01FF0000u && dst <= nds_watch && nds_watch < dst + span)
            nds_log("dma%d %x -> %x, %u mots de %d, cnt=%x arm=%x image %u, source: %8x %8x %8x %8x", i, src, dst, n, step, cnt, nds_pc, nds_frame, REG32(src), REG32(src + 4), REG32(src + 8), REG32(src + 12));
    }
#endif
    if (src < 0x01FF0000u || dst < 0x01FF0000u) {
        /* MI_StopDma lance une copie vide pour arreter le canal */
    } else if (mode == 7|| (dst >= IO + 0x400 && dst < IO + 0x600)) {
        for (u32 k = 0; k < n; k++, src += sstep) gx_fifo_word(*(u32 *)DS(src));
    } else if (w32) {
        for (u32 k = 0; k < n; k++, src += sstep, dst += dstep) *(volatile u32 *)DS(dst) = *(volatile u32 *)DS(src);
    } else {
        for (u32 k = 0; k < n; k++, src += sstep, dst += dstep) *(volatile u16 *)DS(dst) = *(volatile u16 *)DS(src);
    }
    if (((cnt >> 21) & 3) != 3) REG32(base + 4) = dst;
    REG32(base) = src;
    if (!((cnt >> 25) & 1) || mode == 0 || mode == 7) REG32(base + 8) = cnt & 0x7FFFFFFFu;
    dma_on[i] = REG32(base + 8) >> 31;
    if (cnt & (1u << 30)) io_raise(1u << (8 + i));
}

void dma_vblank(void) {
    for (int i = 0; i < 4; i++) {
        u32 cnt = REG32(IO + 0xB8 + i * 12);
        if ((cnt >> 31) && ((cnt >> 27) & 7) == 1) dma_run(i);
    }
}

/* ------------------------------------------------------------------ evenements */
static const u32 tdiv[4] = {1, 64, 256, 1024};
static const u8 tshift[4] = {0, 6, 8, 10};

static void timers_poll(u64 now) {
    for (int i = 0; i < 4; i++) {
        if (!(tm[i].ctrl & 0x80) || !(tm[i].ctrl & 0x40) || (tm[i].ctrl & 4)) continue;
        if (now >= tm[i].next) {
            u64 period = (u64)(0x10000 - tm[i].reload) * tdiv[tm[i].ctrl & 3];
            while (tm[i].next <= now) tm[i].next += period;
            io_raise(1u << (3 + i));
        }
    }
}

void io_poll(void) {
    enum { PERIOD = 560190 }; /* 355 x 263 x 6 cycles : une image */
    static u64 next_vblank = PERIOD * VISIBLE / LINES;
    u64 now = cycles64();
    OPEN();
    timers_poll(now);
    u32 stat = REG16(IO + 4);
    if (now >= next_vblank) {
        gfx_render();
        if (nds_fixed_clock) snd_fixed_frame();
        dma_vblank();
        present_frame();
        if (nds_cb.frame) nds_mods_frame(nds_frame);
        nds_frame++;
        if (stat & 8) io_raise(1);
        arm7_tick();
        next_vblank += PERIOD;
        if (next_vblank <= now) next_vblank = now + PERIOD; /* en retard : pas de rattrapage */
    }
    /* ligne courante, d'apres le prochain VBlank */
    u64 to_vblank = next_vblank - now;
    u32 back = (u32)(to_vblank * LINES / PERIOD);
    u32 line = (VISIBLE + LINES - (back % LINES)) % LINES;
    u32 target = ((stat >> 8) & 0xFF) | ((stat & 0x80) << 1);
    if ((stat & 0x20) && line != line_prev) {
        int crossed = line_prev <= line ? (target > line_prev && target <= line) : (target > line_prev || target <= line);
        if (crossed) io_raise(4);
    }
    if ((stat & 0x10) && line != line_prev) io_raise(2);
    line_prev = line;
    REG16(IO + 6) = (u16)line;
    u32 sub = (u32)(to_vblank * LINES % PERIOD);
    u32 s = stat & 0xFFB8;
    if (line >= VISIBLE && line < LINES - 1) s |= 1;
    if (sub * 256 / PERIOD < 56) s |= 2;
    if (line == target) s |= 4;
    REG16(IO + 4) = (u16)s;
    REG32(IO + 0x214) = if_state;
    CLOSE();
}

extern int nds_trace;
extern u32 nds_trace_eip, nds_trace_addr, nds_trace_stack[13];

void io_wait_irq(void) {
    if (nds_trace) {
        u32 *st = (u32 *)__builtin_frame_address(0); /* pile de l'hote */
        int k = 0;
        for (int i = 0; i < 300 && k < 12; i++)
            if (st[i] >= 0x10000000u && st[i] < 0x11000000u) nds_trace_stack[k++] = st[i];
        nds_trace_stack[k] = 0;
        nds_trace_addr = 0xBEEF;
    }
    for (;;) {
        io_poll();
        OPEN();
        int want = (REG32(IO + 0x210) & if_state) != 0;
        CLOSE();
        if (want) {
            if (!(nds_cpsr & 0x80)) irq_deliver();
            return;
        }
        if (!nds_fixed_clock) os_sleep(1);
    }
}

/* donnees du SDK dans la memoire DS */
typedef struct { u16 need; u16 depth; u32 current; u32 list; u32 cb; } ThreadInfo;
/* SDK des premiers jeux (nds_old_sdk) : current et list un mot plus loin, file des fils en attente = masque de leurs numeros */
typedef struct { u16 need; u16 depth; u32 unknown; u32 current; u32 list; u32 cb; } OldThreadInfo;
GAME_SYM(OSi_ThreadInfo);
GAME_SYM(OSi_IrqThreadQueue);
#define thread_info ((ThreadInfo *)DS(GAME_ADDR(OSi_ThreadInfo)))
#define old_thread_info ((OldThreadInfo *)DS(GAME_ADDR(OSi_ThreadInfo)))
#define thread_current (*(nds_old_sdk ? &old_thread_info->current : &thread_info->current))
#define irq_queue ((u32 *)DS(GAME_ADDR(OSi_IrqThreadQueue))) /* [2] */
extern u64 GAME_FN(OS_SelectThread)();
extern u64 OS_SaveContext();
extern u64 OS_LoadContext();
typedef u64 (*Fn)();

void net_irq(void);

/* Accroche : le jeu attend ici en boucle qu'une interruption change une variable. Sur la console l'interruption arrive d'elle-meme ; ici elle
 * n'arrive que si le code touche un registre, il faut donc lui laisser la place. */
void nds_hook_spin(u32 r0) {
    io_poll();
    if (!in_irq && !(nds_cpsr & 0x80)) irq_deliver();
}

void irq_deliver(void) {
    OPEN();
    while (!in_irq && !(nds_cpsr & 0x80) && (REG32(IO + 0x208) & 1) && (REG32(IO + 0x210) & if_state)) {
        u32 saved = nds_cpsr;
        u32 handler = *(u32 *)DS(nds_dtcm_base() + 0x3FFCu); /* gestionnaire du SDK, ecrit par le jeu */
        if (!handler) break;
        Fn vec = (Fn)nds_call_target(handler);
        in_irq = 1;
        nds_cpsr |= 0x80;
        REG32(IO + 0x214) = if_state;
        CLOSE();
        net_irq();
        if (nds_cb.irq) nds_mods_irq(if_state);
        /* NDS_EH : la pile ARM du fil interrompu est en cours d'usage, l'interruption a la sienne comme sur le DS */
        u32 saved_sp = nds_sp, saved_lr = nds_lr;
        nds_sp = nds_dtcm_base() + 0x3F80u;
        vec(0, 0, 0, 0);
        nds_sp = saved_sp;
        nds_lr = saved_lr;
        OPEN();
        nds_cpsr = saved;
        in_irq = 0;
        /* comme OS_IrqHandler_ThreadSwitch : reveiller ceux qui attendaient une interruption */
        if (nds_old_sdk) {
            u16 mask = *(u16 *)irq_queue;
            if (mask) {
                *(u16 *)irq_queue = 0;
                for (u32 t = old_thread_info->list; t; t = ((u32 *)DS(t))[0x68 / 4]) {
                    u32 *th = (u32 *)DS(t);
                    if (mask >> th[0x6C / 4] & 1) th[0x64 / 4] = 1;
                }
                thread_info->need = 1;
            }
        } else if (irq_queue[0]) {
            u32 t = irq_queue[0];
            while (t) {
                u32 *th = (u32 *)DS(t);
                th[100 / 4] = 1;
                th[120 / 4] = 0;
                th[124 / 4] = 0;
                t = th[128 / 4];
                th[128 / 4] = 0;
            }
            irq_queue[0] = 0;
            irq_queue[1] = 0;
            thread_info->need = 1;
        }
        if (thread_info->need) {
            thread_info->need = 0;
            u32 cur = thread_current;
            u32 next = (u32)GAME_FN(OS_SelectThread)();
            if (cur && next && cur != next) {
                CLOSE();
                if ((u32)OS_SaveContext(cur) == 0) {
                    thread_current = next;
                    OS_LoadContext(next);
                }
                OPEN();
            }
        }
    }
    CLOSE();
}

/* ------------------------------------------------------------------ lectures */
void io_before_read(u32 addr) {
    nds_in_veh = 1;
    u32 r = addr - IO;
    if (r < 0x280 || r >= 0x2C0) io_poll();
    if (addr >= 0x04100000u) {
        if (fifo_count()) fifo_top = fifo_q[fifo_r++ & 255];
        REG32(0x04100000u) = fifo_top;
    } else if (r >= 0x100 && r < 0x110) {
        int i = (r - 0x100) / 4;
        u32 v = tm[i].reload;
        if (tm[i].ctrl & 0x80) {
            u64 ticks = (cycles64() - tm[i].start) >> tshift[tm[i].ctrl & 3];
            u32 span = 0x10000 - tm[i].reload;
            v = tm[i].reload + (u32)(ticks % span);
        }
        REG16(IO + 0x100 + i * 4) = (u16)v;
    } else if (r == 0x130) {
        REG16(IO + 0x130) = (u16)(~present_keys() & 0x3FF);
    } else if (r == 0x184) {
        u32 c = REG32(IO + 0x184) & ~0x4303u;
        c |= 1;
        c |= fifo_count() ? 0 : 0x100;
        REG32(IO + 0x184) = c;
    } else if (r == 0x600) {
        REG32(IO + 0x600) = gx_status();
    }
    nds_in_veh = nds_has_c ? 0 : 1;
}

/* ------------------------------------------------------------------ ecritures */
static u64 udiv64(u64 n, u64 d, u64 *rem) {
    if (!((n | d) >> 32)) {
        u32 a = (u32)n, b = (u32)d;
        if (rem) *rem = a % b;
        return a / b;
    }
    u64 q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) { r -= d; q |= 1ull << i; }
    }
    if (rem) *rem = r;
    return q;
}
u64 __udivdi3(u64 a, u64 b) { return udiv64(a, b, 0); }
u64 __umoddi3(u64 a, u64 b) { u64 r; udiv64(a, b, &r); return r; }
s64 __divdi3(s64 a, s64 b) {
    int neg = (a < 0) ^ (b < 0);
    u64 q = udiv64(a < 0 ? -(u64)a : (u64)a, b < 0 ? -(u64)b : (u64)b, 0);
    return neg ? -(s64)q : (s64)q;
}
s64 __moddi3(s64 a, s64 b) {
    u64 r;
    udiv64(a < 0 ? -(u64)a : (u64)a, b < 0 ? -(u64)b : (u64)b, &r);
    return a < 0 ? -(s64)r : (s64)r;
}

static void divider(void) {
    u32 cnt = REG32(IO + 0x280);
    u32 mode = cnt & 3;
    s64 num = mode == 0 ? (s64)(s32)REG32(IO + 0x290) : (s64)(((u64)REG32(IO + 0x294) << 32) | REG32(IO + 0x290));
    s64 den = mode == 2 ? (s64)(((u64)REG32(IO + 0x29C) << 32) | REG32(IO + 0x298)) : (s64)(s32)REG32(IO + 0x298);
    s64 q, r;
    if (den == 0) {
        q = num < 0 ? 1 : -1;
        r = num;
        cnt |= 0x4000;
    } else {
        cnt &= ~0x4000u;
        if (num == (s64)0x8000000000000000ull && den == -1) { q = num; r = 0; }
        else { q = __divdi3(num, den); r = __moddi3(num, den); }
    }
    REG32(IO + 0x280) = cnt & ~0x8000u;
    REG32(IO + 0x2A0) = (u32)q;
    REG32(IO + 0x2A4) = (u32)((u64)q >> 32);
    REG32(IO + 0x2A8) = (u32)r;
    REG32(IO + 0x2AC) = (u32)((u64)r >> 32);
}

static void sqrt_unit(void) {
    u32 cnt = REG32(IO + 0x2B0);
    u64 v = ((u64)REG32(IO + 0x2BC) << 32) | REG32(IO + 0x2B8);
    if (!(cnt & 1)) v &= 0xFFFFFFFFu;
    u64 x = v, res = 0, bit = 1ull << 62;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= res + bit) { x -= res + bit; res = (res >> 1) + bit; }
        else res >>= 1;
        bit >>= 2;
    }
    REG32(IO + 0x2B4) = (u32)res;
    REG32(IO + 0x2B0) = cnt & ~0x8000u;
}

/* Pilote de carte propre au jeu (Pokemon Dash) : commande 0xB7 sur le bus, mots recus par un DMA en mode carte, interruption a la fin */
static int card_rom_read(void) {
    if (!nds_old_sdk || REG8(IO + 0x1A8) != 0xB7) return 0;
    int channel = -1;
    for (int i = 0; i < 4 && channel < 0; i++) {
        u32 cnt = REG32(IO + 0xB8 + i * 12);
        if ((cnt >> 31) && ((cnt >> 27) & 7) == 5) channel = i;
    }
    if (channel < 0) return 0;
    u32 pos = (u32)REG8(IO + 0x1A9) << 24 | (u32)REG8(IO + 0x1AA) << 16 | (u32)REG8(IO + 0x1AB) << 8 | REG8(IO + 0x1AC);
    u32 blocks = (REG32(IO + 0x1A4) >> 24) & 7;
    u32 size = blocks == 7 ? 4 : blocks ? 0x100u << blocks : 0;
    if (pos + size > nds_rom_size) nds_fatal("lecture de carte hors de la rom : %x + %x", pos, size);
    nds_copy(DS(REG32(IO + 0xB4 + channel * 12)), nds_rom + pos, size);
    if (nds_cb.rom_read) nds_mods_rom_read(pos, size, DS(REG32(IO + 0xB4 + channel * 12)));
    REG32(IO + 0x1A4) &= 0x7F7FFFFFu;
    if (REG8(IO + 0x1A1) & 0x40) io_raise(1u << 19);
    return 1;
}

void io_after_write(u32 addr) {
    nds_in_veh = 1;
    u32 r = addr - IO;
    u32 word = r & ~3u;
    if (r >= 0xB0 && r < 0xE0) {
        int i = (r - 0xB0) / 12;
        u32 cnt = REG32(IO + 0xB8 + i * 12);
        if ((r - 0xB0) % 12 >= 8) {
            u32 mode = (cnt >> 27) & 7;
            int started = (cnt >> 31) && (!dma_on[i] || !nds_old_sdk);
            dma_on[i] = cnt >> 31;
            if (started && (mode == 0 || mode == 7)) dma_run(i);
        }
    } else if (r >= 0x100 && r < 0x110) {
        int i = (r - 0x100) / 4;
        if (!(r & 2)) tm[i].reload = REG16(IO + 0x100 + i * 4);
        u32 ctl = REG16(IO + 0x102 + i * 4);
        if ((ctl & 0x80) && !(tm[i].ctrl & 0x80)) {
            tm[i].start = cycles64();
            tm[i].next = tm[i].start + (u64)(0x10000 - tm[i].reload) * tdiv[ctl & 3];
        }
        tm[i].ctrl = ctl;
    } else if (word == 0x180) {
        u32 out = (REG32(IO + 0x180) >> 8) & 15;
        /* poignee de main du SDK : l'ARM7 repond par la valeur suivante */
        REG32(IO + 0x180) = (REG32(IO + 0x180) & ~0xFu) | ((out + 1) & 15);
    } else if (word == 0x184) {
        if (REG32(IO + 0x184) & 8) fifo_r = fifo_w;
        REG32(IO + 0x184) &= ~0x4008u; /* bits d'action, pas d'etat */
    } else if (word == 0x1A4) {
        /* commande de carte : pret tout de suite. Sans DMA, le seul mot lu ainsi est l'identifiant de la rom */
        if (REG32(IO + 0x1A4) >> 31 && !card_rom_read()) {
            REG32(IO + 0x1A4) = (REG32(IO + 0x1A4) & 0x7FFFFFFFu) | (1u << 23);
            REG32(0x04100010u) = 0x00000FC2u;
        }
    } else if (word == 0x188) {
        arm7_fifo_send(REG32(IO + 0x188));
    } else if (word == 0x214) {
        if_state &= ~REG32(IO + 0x214);
        REG32(IO + 0x214) = if_state;
    } else if (r >= 0x240 && r < 0x24C) {
        vram_cnt_changed();
    } else if (word == 0x280 || (r >= 0x290 && r < 0x2A0)) {
        divider();
    } else if (word == 0x2B0 || (r >= 0x2B8 && r < 0x2C0)) {
        sqrt_unit();
    } else if (r >= 0x400 && r < 0x440) {
        gx_fifo_word(REG32(addr & ~3u));
    } else if (r >= 0x440 && r < 0x600) {
        gx_write(addr & ~3u, REG32(addr & ~3u));
    }
    /* la reponse de l'ARM7 peut lever l'interruption de reception */
    if (fifo_count() && (REG32(IO + 0x184) & 0x400)) io_raise(1u << 18);
    nds_in_veh = nds_has_c ? 0 : 1;
}

u32 nds_io_read(u32 a, u32 size) {
    if ((s32)a < 0) return 0; /* BIOS et adresses hors memoire : pas de faute sur le DS */
    if (a < 0x01000000u) {
        /* sur le DS le bas de la memoire est un miroir de l'ITCM : un pointeur nul se lit sans planter */
        u32 m = 0x01FF8000u + (a & 0x7FFF);
        return size == 4 ? REG32(m) : size == 2 ? REG16(m) : REG8(m);
    }
    io_before_read(a);
    if (!in_irq && !(nds_cpsr & 0x80)) irq_deliver();
    u32 v = size == 4 ? REG32(a) : size == 2 ? REG16(a) : REG8(a);
    return nds_cb.io_read ? nds_mods_io_read(a, size, v) : v;
}

void nds_io_write(u32 a, u32 v, u32 size) {
    if (a < 0x01000000u || (s32)a < 0) return;
    if (nds_cb.io_write && nds_mods_io_write(a, size, &v)) return;
    if (size == 4) REG32(a) = v;
    else if (size == 2) REG16(a) = (u16)v;
    else REG8(a) = (u8)v;
    io_after_write(a);
    if (!in_irq && !(nds_cpsr & 0x80)) irq_deliver();
}

void io_init(void) {
    freq = os_time_freq();
    t0 = os_time_now();
    clock_k = (u32)((MS_FREQ << 16) / (u64)freq);
    os_time_hires();
    nds_in_veh = nds_has_c ? 0 : 1;
    REG32(IO + 0x184) = 0x101;
    REG32(0x027FFF8Cu) = 0xFFFFFFFFu; /* tous les modules ARM7 sont prets */
    /* identifiant de carte attendu : sinon le SDK croit la carte retiree */
    REG32(0x027FFC00u) = 0x00000FC2u;
    REG32(0x027FF800u) = 0x00000FC2u;
    REG32(0x027FFC3Cu) = 0x00000FC2u;
    /* --ds-lang n : langue de la console (0 japonais, 1 anglais, 2 francais, 3 allemand, 4 italien, 5 espagnol), lue par
     * OS_GetLanguage dans les reglages de l'utilisateur (0x027FFC80 + 0x64). Sans l'option, la memoire reste telle quelle. */
    const char *lang;
    if (nds_arg("--ds-lang", &lang) && lang && *lang >= '0' && *lang <= '5') REG8(0x027FFCE4u) = (u8)((REG8(0x027FFCE4u) & ~7) | (*lang - '0'));
}
