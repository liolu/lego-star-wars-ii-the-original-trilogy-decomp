/* Demarrage : memoire DS (fenetre nds_mem), chargement de la rom, exceptions pour les registres, BIOS, threads. */
#include "nitro.h"

/* sans en-tete systeme : la boite a outils du launcher n'en livre aucun */
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

u32 nds_cpsr = 0x1F;
u32 nds_pc, nds_r[6]; /* derniere instruction ARM et registres, dans les versions de debug */
u8 *nds_rom;
u32 nds_rom_size;
#if !NDS_IDENTITY
uptr nds_mem; /* base de la fenetre DS, fixee par nds_start */
#endif

extern void *const nds_entry_fn;

/* ------------------------------------------------------------------ petites fonctions sans libc */
#if defined(__i386__) || defined(_WIN32)
void nds_copy(void *dst, const void *src, u32 n) {
    __asm__ volatile("rep movsb" : "+D"(dst), "+S"(src), "+c"(n) : : "memory");
}
void nds_zero(void *dst, u32 n) {
    __asm__ volatile("rep stosb" : "+D"(dst), "+c"(n) : "a"(0) : "memory");
}
#else /* Mac : libSystem fournit memcpy/memset */
void nds_copy(void *dst, const void *src, u32 n) { __builtin_memcpy(dst, src, n); }
void nds_zero(void *dst, u32 n) { __builtin_memset(dst, 0, n); }
#endif

static int fmt_u(char *o, u32 v, int base, int width) {
    char tmp[16];
    int n = 0, w = 0;
    do { tmp[n++] = "0123456789abcdef"[v % base]; v /= base; } while (v);
    while (n < width) tmp[n++] = '0';
    while (n) o[w++] = tmp[--n];
    return w;
}

static void vlog(const char *fmt, va_list ap) {
    char buf[1024];
    int w = 0;
    for (; *fmt && w < 900; fmt++) {
        if (*fmt != '%') { buf[w++] = *fmt; continue; }
        fmt++;
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + *fmt++ - '0';
        if (*fmt == 'x') w += fmt_u(buf + w, va_arg(ap, u32), 16, width);
        else if (*fmt == 'd') {
            int v = va_arg(ap, int);
            if (v < 0) { buf[w++] = '-'; v = -v; }
            w += fmt_u(buf + w, v, 10, width);
        } else if (*fmt == 'u') w += fmt_u(buf + w, va_arg(ap, u32), 10, width);
        else if (*fmt == 's') { const char *s = va_arg(ap, const char *); while (s && *s && w < 900) buf[w++] = *s++; }
        else buf[w++] = *fmt;
    }
    buf[w++] = '\n';
    os_stderr_write(buf, w);
}

void nds_log(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vlog(fmt, ap); va_end(ap); }

void nds_vlog(const char *fmt, va_list ap) { vlog(fmt, ap); }

void nds_fatal(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vlog(fmt, ap);
    va_end(ap);
#ifdef NDS_INSTRUMENT
    { extern u32 nds_ring[4096], nds_ring_pos; extern u32 nds_pc; nds_log("arm=%x, derniers appels :", nds_pc);
      for (int i = 4096 - 120; i < 4096; i++) nds_log("  appel %8x", nds_ring[(nds_ring_pos + i) & 4095]); }
#endif
    nds_mods_crash_note();
    os_exit(3);
}

void nds_unhandled(const char *f, u32 pc) { nds_fatal("instruction non traduite %s @%x", f, pc); }
void nds_missing(const char *name) { nds_fatal("fonction absente : %s", name); }

/* ------------------------------------------------------------------ arguments */
static const char *argv_[32];
static int argc_;

static void parse_args(void) {
    argc_ = os_get_args(argv_, 32);
}

int nds_arg(const char *name, const char **value) {
    for (int i = 1; i < argc_; i++) {
        const char *a = argv_[i], *b = name;
        while (*a && *a == *b) a++, b++;
        if (*a == 0 && *b == 0) {
            if (value) *value = i + 1 < argc_ ? argv_[i + 1] : 0;
            return 1;
        }
    }
    return 0;
}

#ifdef NDS_INSTRUMENT
/* --watch adresse : qui ecrit a ce mot de memoire (ecritures du code traduit) */
u32 nds_watch = 1;
void nds_watch_hit(u32 a, u32 v) {
    static u32 hits;
    if (hits++ < 12) nds_log("ecriture %x = %x par arm=%x, r0=%x r1=%x r2=%x r3=%x image %u", a, v, nds_pc, nds_r[0], nds_r[1], nds_r[2], nds_r[3], nds_frame);
}
u32 nds_ring[4096], nds_ring_pos;
void __cyg_profile_func_enter(void *f, void *c) { nds_ring[nds_ring_pos++ & 4095] = (u32)(uptr)f; } /* 64 bits : 32 bits bas */
void __cyg_profile_func_exit(void *f, void *c) {}
#if !NDS_IDENTITY
/* HOST2DS des versions instrumentees : un pointeur hors de la fenetre veut dire du code du jeu sur une pile etrangere */
u32 nds_host2ds(const volatile void *p) {
    uptr d = (uptr)p - nds_mem;
    if (d > 0xFFFFFFFFu) nds_fatal("pointeur hors de la memoire DS : %x%8x (fenetre %x%8x) arm=%x", (u32)((u64)(uptr)p >> 32), (u32)(uptr)p, (u32)((u64)nds_mem >> 32), (u32)nds_mem, nds_pc);
    return (u32)d;
}
#endif
#endif

/* ------------------------------------------------------------------ CP15 */
static u32 cp15_dtcm = 0x027E000Au, cp15_ctrl = 0x00052078u;

u32 nds_cp15_read(u32 crn, u32 crm, u32 op2) {
    if (crn == 1) return cp15_ctrl;
    if (crn == 9 && crm == 1) return op2 == 0 ? cp15_dtcm : 0x20u;
    if (crn == 0) return op2 == 1 ? 0x0F0D2112u : 0x41059461u;
    return 0;
}

u32 nds_dtcm_base(void) { return cp15_dtcm & 0xFFFFF000u; }

void nds_cp15_write(u32 crn, u32 crm, u32 op2, u32 v) {
    if (crn == 1) cp15_ctrl = v;
    else if (crn == 9 && crm == 1 && op2 == 0) cp15_dtcm = v;
    else if (crn == 7 && crm == 0 && op2 == 4) io_wait_irq(); /* attente d'interruption */
}

void nds_cpsr_changed(void) {
    /* un jeu peut attendre un VBlank sans toucher aux registres : le temps avance aussi ici */
    if (!(nds_cpsr & 0x80)) {
        io_poll();
        irq_deliver();
    }
}

/* ------------------------------------------------------------------ BIOS (swi) */
/* src, dst : adresses DS */
static u32 lz77(u32 src, u32 dst, int vram) {
    u32 head = *(u32 *)DS(src);
    u32 left = head >> 8, s = src + 4, d = dst;
    while (left) {
        u8 flags = *DS(s++);
        for (int i = 0; i < 8 && left; i++, flags <<= 1) {
            if (flags & 0x80) {
                u32 b = (*DS(s) << 8) | *DS(s + 1);
                s += 2;
                u32 n = (b >> 12) + 3, off = (b & 0xFFF) + 1;
                for (u32 k = 0; k < n && left; k++, left--, d++) {
                    u8 v = *DS(d - off);
                    if (vram) {
                        u16 *p = (u16 *)DS(d & ~1u);
                        *p = (d & 1) ? (*p & 0xFF) | (v << 8) : (*p & 0xFF00) | v;
                    } else *DS(d) = v;
                }
            } else {
                u8 v = *DS(s++);
                if (vram) {
                    u16 *p = (u16 *)DS(d & ~1u);
                    *p = (d & 1) ? (*p & 0xFF) | (v << 8) : (*p & 0xFF00) | v;
                } else *DS(d) = v;
                d++; left--;
            }
        }
    }
    return head >> 8;
}

static void rl_uncomp(u32 src, u32 dst, int vram) {
    u32 left = *(u32 *)DS(src) >> 8, s = src + 4, d = dst;
    while (left) {
        u8 c = *DS(s++);
        u32 n = (c & 0x80) ? (c & 0x7F) + 3 : (c & 0x7F) + 1;
        u8 rep = (c & 0x80) ? *DS(s++) : 0;
        for (u32 k = 0; k < n && left; k++, left--, d++) {
            u8 v = (c & 0x80) ? rep : *DS(s++);
            if (vram) {
                u16 *p = (u16 *)DS(d & ~1u);
                *p = (d & 1) ? (*p & 0xFF) | (v << 8) : (*p & 0xFF00) | v;
            } else *DS(d) = v;
        }
    }
}

static void huff_uncomp(u32 src, u32 dst) {
    u32 head = *(u32 *)DS(src);
    u32 left = head >> 8, bits = head & 15;
    u32 tree = src + 4; /* noeuds : adresses DS (l'alignement porte sur l'adresse DS) */
    u32 *in = (u32 *)DS(src + 4 + (*DS(tree) + 1) * 2);
    u32 acc = 0, acc_bits = 0, node = 0;
    u32 cur = tree + 1;
    (void)node;
    u32 out = 0, out_bits = 0;
    u8 *d = DS(dst);
    while (left) {
        u32 word = *in++;
        for (int i = 31; i >= 0 && left; i--) {
            u32 b = (word >> i) & 1;
            u8 v = *DS(cur);
            u32 next = (cur & ~1u) + (v & 0x3F) * 2 + 2;
            u32 is_leaf = b ? (v & 0x40) : (v & 0x80);
            cur = next + b;
            if (is_leaf) {
                out |= (u32)*DS(cur) << out_bits;
                out_bits += bits;
                cur = tree + 1;
                if (out_bits >= 8) { *d++ = (u8)out; out >>= 8; out_bits -= 8; left--; }
            }
        }
    }
    (void)acc; (void)acc_bits;
}

static void bit_unpack(u32 src, u32 dst, u32 info) {
    u16 len = *(u16 *)DS(info);
    u8 sw = *DS(info + 2), dw = *DS(info + 3);
    u32 off = *(u32 *)DS(info + 4);
    int zero = off >> 31;
    off &= 0x7FFFFFFF;
    u32 acc = 0, acc_bits = 0, out = dst;
    for (u32 i = 0; i < len; i++) {
        u8 b = *DS(src + i);
        for (u32 k = 0; k < 8; k += sw) {
            u32 v = (b >> k) & ((1u << sw) - 1);
            if (v || zero) v += off;
            acc |= v << acc_bits;
            acc_bits += dw;
            if (acc_bits >= 32) { *(u32 *)DS(out) = acc; out += 4; acc = 0; acc_bits = 0; }
        }
    }
}

static u32 isqrt(u32 x) {
    u32 r = 0, b = 1u << 30;
    while (b > x) b >>= 2;
    while (b) {
        if (x >= r + b) { x -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

void nds_swi(u32 n, u32 *r0, u32 *r1, u32 *r2, u32 *r3) {
#ifdef NDS_INSTRUMENT
    if (n >= 0x0B && n != 0x0D && n != 0x0E && *r1 <= nds_watch && nds_watch < *r1 + 0x10000) nds_log("swi %x %x -> %x r2=%x arm=%x", n, *r0, *r1, *r2, nds_pc);
#endif
    switch (n) {
    case 0x03: break;                                     /* WaitByLoop */
    case 0x04: case 0x05: {                               /* IntrWait, VBlankIntrWait */
        u32 mask = n == 5 ? 1 : *r1;
        volatile u32 *check = (volatile u32 *)DS(nds_dtcm_base() + 0x3FF8u);
        if (n == 5 || *r0) *check &= ~mask;
        u32 saved = nds_cpsr;
        nds_cpsr &= ~0x80u;
        while (!(*check & mask)) io_wait_irq();
        *check &= ~mask;
        nds_cpsr = saved;
        break;
    }
    case 0x06: io_wait_irq(); break;                      /* Halt */
    case 0x09: {                                          /* Div */
        s32 num = (s32)*r0, den = (s32)*r1;
        if (den == 0) { *r0 = num < 0 ? -1 : 1; *r1 = num; *r3 = 1; break; }
        s32 q = (num == (s32)0x80000000 && den == -1) ? num : num / den;
        *r0 = q; *r1 = num - q * den; *r3 = q < 0 ? -q : q;
        break;
    }
    case 0x0B: {                                          /* CpuSet */
        u32 s = *r0, d = *r1, c = *r2, count = c & 0x1FFFFF;
        if (c & (1 << 26)) {
            u32 v = c & (1 << 24) ? *(u32 *)DS(s) : 0;
            for (u32 i = 0; i < count; i++, d += 4, s += (c & (1 << 24)) ? 0 : 4) *(u32 *)DS(d) = (c & (1 << 24)) ? v : *(u32 *)DS(s);
        } else {
            u16 v = *(u16 *)DS(s);
            for (u32 i = 0; i < count; i++, d += 2, s += (c & (1 << 24)) ? 0 : 2) *(u16 *)DS(d) = (c & (1 << 24)) ? v : *(u16 *)DS(s);
        }
        break;
    }
    case 0x0C: {                                          /* CpuFastSet */
        u32 s = *r0, d = *r1, c = *r2, count = (c & 0x1FFFFF);
        count = (count + 7) & ~7u;
        u32 v = *(u32 *)DS(s);
        for (u32 i = 0; i < count; i++, d += 4, s += 4) *(u32 *)DS(d) = (c & (1 << 24)) ? v : *(u32 *)DS(s);
        break;
    }
    case 0x0D: *r0 = isqrt(*r0); break;                   /* Sqrt */
    case 0x0E: {                                          /* GetCRC16 */
        static const u16 tab[16] = {0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401,0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400};
        u32 crc = *r0 & 0xFFFF;
        u8 *p = DS(*r1);
        for (u32 i = 0; i < *r2; i += 2) {
            u32 w = p[i] | (p[i + 1] << 8);
            for (int k = 0; k < 4; k++, w >>= 4) {
                u32 t = tab[crc & 15];
                crc = (crc >> 4) ^ t ^ tab[w & 15];
            }
        }
        *r0 = crc;
        break;
    }
    case 0x10: bit_unpack(*r0, *r1, *r2); break;
    case 0x11: lz77(*r0, *r1, 0); break;
    case 0x12: lz77(*r0, *r1, 1); break;
    case 0x13: huff_uncomp(*r0, *r1); break;
    case 0x14: rl_uncomp(*r0, *r1, 0); break;
    case 0x15: rl_uncomp(*r0, *r1, 1); break;
    default: nds_fatal("swi %x inconnu", n);
    }
}

/* ------------------------------------------------------------------ threads : contexte de l'hote dans context.c */
typedef u64 (*Fn)();

/* piles de l'hote dans la fenetre, au-dessus de la zone DS, jamais sous 0x01000000 que le code traduit reserve au miroir ITCM.
 * Le code traduit y prend sp = HOST2DS(&stack_[n]) : tout code du jeu doit tourner sur une de ces piles. */
static u32 next_stack = 0x20000000u; /* adresse DS */
static u8 *stack_alloc(u32 size) {
    u8 *p = os_mem_alloc(DS(next_stack), size);
    /* une zone deja prise (pile d'un fil de l'hote, bibliotheque) : on essaie plus loin */
    extern const u32 nds_stack_reuse;
    for (int tries = 0; !p && nds_stack_reuse && tries < 512; tries++) {
        next_stack += 0x100000u;
        p = os_mem_alloc(DS(next_stack), size);
    }
    if (!p) nds_fatal("pile impossible a %x", next_stack);
    next_stack += (size + 0xFFFF) & ~0xFFFFu;
    return p;
}

static void thread_tramp(u32 ctx) {
    u32 *c = (u32 *)DS(ctx);
    /* OSContext : cpsr, r0..r12, sp, lr, pc+4 */
    u32 arg = c[1], lr = c[15];
    nds_sp = c[14];
    nds_lr = 0xFFFFFFFEu;
    ((Fn)nds_call_target(nds_ctx_func(ctx)))(arg, 0, 0, 0);
    if (lr) ((Fn)nds_call_target(lr))(0, 0, 0, 0);
    nds_fatal("fin de thread sans suite");
}

void OS_InitContext(u32 ctx, u32 pc, u32 sp) {
    u32 *c = (u32 *)DS(ctx);
    enum { STACK = 1 << 20 };
    extern u8 *nds_ctx_reuse(u32 ctx, u32 size);
    u8 *st = nds_ctx_reuse(ctx, STACK);
    nds_ctx_init(ctx, st ? st : stack_alloc(STACK), STACK, thread_tramp, pc);
    nds_zero(c, 0x48);
    c[0] = 0x1F;
    c[14] = sp;
    c[16] = pc + 4;
}

/* ------------------------------------------------------------------ exceptions : registres a effets */
int nds_trace;
u32 nds_trace_count, nds_trace_eip, nds_trace_addr, nds_trace_stack[13];
static u32 pending_page, pending_addr, pending_write;
void nds_irq_tramp(void);
#if defined(__i386__)
__asm__(".globl _nds_irq_tramp\n_nds_irq_tramp:\n  pushf\n  pusha\n  cld\n  call _irq_deliver\n  popa\n  popf\n  ret\n");
#else /* seulement pour --with-c (piege des registres), 32 bits seulement : MACOS.md 3.5 */
void nds_irq_tramp(void) { nds_fatal("piege des registres sans version 64 bits"); }
#endif

static void io_protect(u32 page, int on) {
    os_mem_protect(DS(page), 0x1000, on ? OS_NOACCESS : OS_RW);
}

static int on_exception(OsFault *e) {
    if (e->kind == OS_FAULT_ACCESS) {
        u32 addr = (u32)(e->addr - nds_mem); /* adresse DS ; hors fenetre, la valeur ne sert qu'au journal */
        u32 page = addr & ~0xFFFu;
        if (page == IO || page == 0x04100000u) {
            io_protect(IO, 0);
            io_protect(0x04100000u, 0);
            if (nds_trace) {
                nds_trace_count++;
                nds_trace_eip = (u32)e->ip;
                nds_trace_addr = addr;
                u32 *st = (u32 *)os_fault_reg(e, OS_REG_SP);
                int k = 0;
                for (int i = 0; i < 200 && k < 12; i++)
                    if (st[i] >= 0x10000000u && st[i] < 0x11000000u) nds_trace_stack[k++] = st[i];
                nds_trace_stack[k] = 0;
            }
            pending_page = page;
            pending_addr = addr;
            pending_write = e->access == 1;
            if (!pending_write) io_before_read(addr);
            os_fault_set_step(e, 1);
            return OS_FAULT_RESUME;
        }
        nds_log("acces memoire invalide %x (%s) eip=%x esp=%x arm=%x", addr, e->access ? "ecriture" : "lecture", (u32)e->ip, (u32)os_fault_reg(e, OS_REG_SP), nds_pc);
#ifdef NDS_INSTRUMENT
        nds_log("  r0=%x r1=%x r2=%x r3=%x r4=%x r5=%x", nds_r[0], nds_r[1], nds_r[2], nds_r[3], nds_r[4], nds_r[5]);
        for (int k = 0; k < 6; k++) {
            u32 a = nds_r[k] & ~3u;
            if (a >= 0x02000000u && a < 0x02400000u) {
                const u32 *m = (const u32 *)DS(a);
                nds_log("  [r%d] %8x %8x %8x %8x %8x %8x %8x %8x", k, m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7]);
            }
        }
#endif
        u32 *st = (u32 *)os_fault_reg(e, OS_REG_SP);
        u32 *bp = (u32 *)os_fault_reg(e, OS_REG_BP);
#ifdef NDS_INSTRUMENT
        {
            extern u32 nds_ring[4096], nds_ring_pos;
            nds_log("  derniers appels :");
            for (int i = 4096 - 1500; i < 4096; i++) nds_log("    %8x", nds_ring[(nds_ring_pos + i) & 4095]);
        }
#endif
        nds_log("  ebp=%x retours :", (u32)(uptr)bp);
        if (nds_last_load_sp) {
            nds_log("  pile du dernier contexte charge %x :", (u32)nds_last_load_sp);
            for (int i = 0; i < 400; i++) {
                u32 v = ((u32 *)nds_last_load_sp)[i];
                if (v >= 0x10000000u && v < 0x11000000u) nds_log("    %8x", v);
            }
            nds_log("  fin");
        }
        for (int i = 0; i < 40; i++) {
            u32 v = st[i];
            if (v >= 0x10000000u && v < 0x11000000u) nds_log("    %x", v);
        }
        nds_mods_crash_note();
        os_exit(4);
    }
    if (e->kind == OS_FAULT_STEP && pending_page) {
        u32 page = pending_page, addr = pending_addr;
        pending_page = 0;
        if (pending_write) io_after_write(addr);
        int ready = io_irq_ready();
        io_protect(IO, 1);
        io_protect(0x04100000u, 1);
        os_fault_set_step(e, 0);
        if (ready) os_fault_call(e, nds_irq_tramp);
        return OS_FAULT_RESUME;
    }
    nds_log("exception %x a %x", e->code, e->ip);
    nds_mods_crash_note();
    os_exit(5);
    return OS_FAULT_PASS;
}

/* ------------------------------------------------------------------ chargement */
static void reserve(u32 addr, u32 size) {
    void *p = os_mem_alloc(DS(addr), size);
    if (p != (void *)DS(addr)) nds_fatal("memoire DS indisponible a %x", addr);
}

static void load_file(const char *path) {
    nds_rom = os_file_load(path, &nds_rom_size);
    if (!nds_rom) nds_fatal("rom introuvable : %s", path);
}

/* decompression BLZ de arm9 (identique au MIi_UncompressBackward du SDK, faite ici car le module charge avant _start) */
static void blz_uncompress(u32 end) {
    u32 sizes = *(u32 *)DS(end - 8), extra = *(u32 *)DS(end - 4);
    u32 start = end - (sizes >> 24);
    u32 stop = end - (sizes & 0xFFFFFF);
    u8 *src = DS(start), *dst = DS(end + extra), *lim = DS(stop);
    while (src > lim) {
        u8 flags = *--src;
        for (int i = 0; i < 8 && src > lim; i++, flags <<= 1) {
            if (flags & 0x80) {
                u32 b = *--src << 8;
                b |= *--src;
                u32 n = (b >> 12) + 3, off = (b & 0xFFF) + 3;
                while (n--) { dst--; *dst = dst[off]; }
            } else { *--dst = *--src; }
        }
    }
}

extern const u32 nds_build_info_offset;

static void load_arm9(void) {
    u8 *h = nds_rom;
    u32 off = *(u32 *)(h + 0x20), ram = *(u32 *)(h + 0x28), size = *(u32 *)(h + 0x2C);
    nds_copy(DS(ram), nds_rom + off, size);
    nds_copy(DS(0x027FFE00u), h, 0x170); /* en-tete de carte, lu par le SDK pour la FAT */
    u32 *info = (u32 *)DS(ram + nds_build_info_offset);
    if (info[5]) {
        blz_uncompress(info[5]);
        info[5] = 0;
    }
    /* les pointeurs de fonctions du jeu restent des adresses ARM : nds_call_target les resout a l'appel */
}

/* profil par echantillons (--prof) : ou le jeu passe son temps */
static void *main_thread;

static void prof_thread(void *arg) {
    static uptr addrs[256];
    static u32 counts[256];
    u32 total = 0;
    uptr sp = 0;
    for (;;) {
        os_sleep(5);
        uptr eip;
        os_thread_sample(main_thread, &eip, &sp);
        eip &= ~0xFu;
        total++;
        for (int i = 0; i < 256; i++) {
            if (addrs[i] == eip) { counts[i]++; break; }
            if (!addrs[i]) { addrs[i] = eip; counts[i] = 1; break; }
        }
        if (total % 1000 == 0) {
            nds_log("profil (%u echantillons, image %u) :", total, nds_frame);
#ifdef NDS_INSTRUMENT
            if (total == 2000) {
                extern u32 nds_ring[4096], nds_ring_pos;
                u32 pos = nds_ring_pos;
                for (int i = 0; i < 4096; i++) nds_log("  appel %8x", nds_ring[(pos + i) & 4095]);
            }
#endif
            {
                u32 *st = (u32 *)sp;
                int shown = 0;
                for (u32 i = 0; i < 200 && shown < 24; i++)
                    if (st[i] >= 0x10000000u && st[i] < 0x11000000u) { nds_log("  pile %8x", st[i]); shown++; }
            }
            for (int k = 0; k < 14; k++) {
                int best = -1;
                for (int i = 0; i < 256; i++) if (counts[i] && (best < 0 || counts[i] > counts[best])) best = i;
                if (best < 0) break;
                nds_log("  %u %8x", counts[best], (u32)addrs[best]);
                counts[best] = 0;
            }
            for (int i = 0; i < 256; i++) { addrs[i] = 0; counts[i] = 0; }
        }
    }
}

static void run_game(void) {
    ((Fn)nds_entry_fn)(0, 0, 0, 0);
    nds_fatal("le jeu s'est termine");
}

void nds_start(void) {
    parse_args();
    nds_fixed_clock = nds_arg("--fixed-clock", 0);
    os_fault_install(on_exception);
#if !NDS_IDENTITY
    nds_mem = (uptr)os_mem_window();
    if (!nds_mem) nds_fatal("fenetre de memoire DS impossible a reserver");
#endif
    reserve(0x01FF0000u, 0x10000);   /* ITCM (0x01FF8000), reservation alignee sur 64 Ko */
    reserve(0x02000000u, 0x02000000u); /* RAM principale, miroirs, WRAM partagee et ARM7 */
    reserve(0x04000000u, 0x00200000u); /* registres */
    reserve(0x05000000u, 0x00001000u); /* palettes */
    reserve(0x06000000u, 0x00A00000u); /* VRAM */
    reserve(0x07000000u, 0x00001000u); /* OAM */
    reserve(0x08000000u, 0x02010000u); /* slot GBA : vide */
    const char *rom = argc_ > 1 ? argv_[1] : 0;
    if (!rom) nds_fatal("usage : jeu.exe <rom.nds>");
    load_file(rom);
    nds_code_map_init();
    load_arm9();
    arm7_init();
    io_init();
    present_init();
    nds_mods_load(); /* apres present_init : langue et messages vers le launcher */
    {
        extern const int nds_has_c;
        if (nds_has_c) {
            io_protect(IO, 1);
            io_protect(0x04100000u, 1);
        }
    }
    nds_log("demarrage %x", (u32)(uptr)nds_entry_fn);
#ifdef NDS_INSTRUMENT
    {
        const char *w;
        if (nds_arg("--watch", &w) && w) {
            u32 v = 0;
            for (; *w; w++) v = v * 16 + (*w >= 'a' ? *w - 'a' + 10 : *w - '0');
            nds_watch = v & ~3u;
        }
    }
#endif
    if (nds_arg("--prof", 0)) {
        main_thread = os_thread_self();
        os_thread_start(prof_thread, 0, 0x10000);
    }
    /* grande pile, allouee apres la memoire DS pour ne pas la gener */
    enum { MAIN_STACK = 16 << 20 };
    u8 *stack = stack_alloc(MAIN_STACK);
    os_set_stack_bounds((uptr)stack, (uptr)(stack + MAIN_STACK));
    /* marge : une fonction traduite lit ses STACK_ARGS arguments, jusqu'a 32 mots ("stack_args") ; 64 octets par defaut */
    extern const u32 nds_stack_margin;
    nds_run_on_stack(stack + MAIN_STACK - (nds_stack_margin ? nds_stack_margin : 64), run_game);
    nds_fatal("le jeu s'est termine");
}
