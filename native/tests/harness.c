/* Test sans rom : PRELUDE de arm2c.py (prelude.h), fonctions de arm2c_cases.s traduites (lifted.c),
 * nds_call_target (nitro-pc/calltab.c) et les fils verts (nitro-pc/context.c). Compile et lance par arm2c_test.py, qui genere prelude.h, cases.h et lifted.c. */
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prelude.h"
#include "cases.h"

void nds_code_map_init(void);
/* nitro-pc/context.c */
u64 OS_SaveContext(u32 ctx);
u64 OS_LoadContext(u32 ctx);
void nds_ctx_init(u32 ctx, u8 *stack, u32 size, void (*entry)(u32), u32 func);
void nds_run_on_stack(u8 *top, void (*fn)(void));

#if !NDS_IDENTITY
uptr nds_mem;
#endif
u32 nds_pc, nds_r[6], nds_cpsr = 0x1F;
const u32 nds_stack_margin = 0, nds_stack_reuse = 0; /* valeurs de patches.json (tables.c des jeux) lues par context.c */

/* ------------------------------------------------------------------ verifications */
static int checks, failures;

static void check_eq(u64 got, u64 want, const char *what, int line) {
    checks++;
    if (got == want) return;
    failures++;
    printf("ECHEC ligne %d : %s = 0x%08x%08x, attendu 0x%08x%08x\n", line, what, (u32)(got >> 32), (u32)got,
           (u32)(want >> 32), (u32)want);
}
#define CHECK_EQ(a, b) check_eq((u64)(a), (u64)(b), #a, __LINE__)

/* nds_fatal : attendu par certains tests (adresse sans fonction), sinon le test s'arrete */
static jmp_buf fatal_jmp;
static volatile int fatal_armed;
void nds_fatal(const char *fmt, ...) {
    if (fatal_armed) {
        fatal_armed = 0;
        longjmp(fatal_jmp, 1);
    }
    va_list ap;
    va_start(ap, fmt);
    printf("fatal : ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    exit(3);
}
#define EXPECT_FATAL(expr) do { \
        fatal_armed = 1; \
        if (!setjmp(fatal_jmp)) { (void)(expr); fatal_armed = 0; check_eq(0, 1, "fatal attendu : " #expr, __LINE__); } \
        else check_eq(1, 1, #expr, __LINE__); \
    } while (0)

/* ce que context.c prend a la couche systeme : les bornes de pile ne servent qu'aux exceptions, sans objet ici */
void os_stack_bounds(uptr *lo, uptr *hi) { *lo = *hi = 0; }
void os_set_stack_bounds(uptr lo, uptr hi) { (void)lo; (void)hi; }

/* ce que context.c prend a core.c */
void nds_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}
void nds_zero(void *dst, u32 n) { memset(dst, 0, n); }

/* ------------------------------------------------------------------ ce que le code traduit demande au runtime */
void nds_cpsr_changed(void) {}
void nds_unhandled(const char *f, u32 pc) { nds_fatal("instruction non traduite %s @%x", f, pc); }
u32 nds_cp15_read(u32 crn, u32 crm, u32 op2) { return 0; }
void nds_cp15_write(u32 crn, u32 crm, u32 op2, u32 v) {}
void nds_swi(u32 n, u32 *r0, u32 *r1, u32 *r2, u32 *r3) { nds_fatal("swi %x", n); }

/* registres (0x04xxxxxx) : valeur reconnaissable. Le reste de ce que IS_IO envoie ici est de la memoire : sous Windows,
 * la pile du fil principal est sous 0x01000000, que IS_IO prend pour le miroir ITCM. */
static u32 io_reads, io_writes, io_last_addr, io_last_value;
u32 nds_io_read(u32 a, u32 size) {
    if ((a >> 24) != 4) return size == 4 ? *(u32 *)DS(a) : size == 2 ? *(u16 *)DS(a) : *DS(a);
    io_reads++;
    return 0xC0DE0000u | (a & 0xFFFF);
}
void nds_io_write(u32 a, u32 v, u32 size) {
    if ((a >> 24) != 4) {
        if (size == 4) *(u32 *)DS(a) = v;
        else if (size == 2) *(u16 *)DS(a) = (u16)v;
        else *DS(a) = (u8)v;
        return;
    }
    io_writes++;
    io_last_addr = a;
    io_last_value = v;
}

#ifdef NDS_INSTRUMENT
u32 nds_watch = 1;
void nds_watch_hit(u32 a, u32 v) {}
#if !NDS_IDENTITY
u32 nds_host2ds(const volatile void *p) {
    uptr d = (uptr)p - nds_mem;
    if (d > 0xFFFFFFFFu) nds_fatal("pointeur hors de la memoire DS : code traduit sur une pile hors de la fenetre");
    return (u32)d;
}
#endif
#endif

/* fonctions C appelees par le code traduit (h_pack10 : 10 arguments, comme t_pack10) */
static u32 host_calls;
u64 h_pack10(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h, u32 i, u32 j) {
    host_calls++;
    return (u64)(i | j << 4) << 32 | (a | b << 4 | c << 8 | d << 12 | e << 16 | f << 20 | g << 24 | h << 28);
}
u64 h_itcm(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h, u32 i, u32 j) { return a + 1; }
u64 h_dtcm(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h, u32 i, u32 j) { return a + 2; }

#define C1(f, a) f(a, 0, 0, 0, 0, 0, 0, 0, 0, 0)
#define C2(f, a, b) f(a, b, 0, 0, 0, 0, 0, 0, 0, 0)
#define PACK10 0x000000A987654321ull /* 1..10, un quartet chacun, r0:r1 */

/* ------------------------------------------------------------------ tests */
static void test_memory(void) {
    WR32(0x02100000u, 0x11223344u);
    CHECK_EQ(RD32(0x02100000u), 0x11223344u);
    CHECK_EQ(RD16(0x02100002u), 0x1122u);
    CHECK_EQ(RD8(0x02100003u), 0x11u);
    CHECK_EQ(LDRW(0x02100000u), 0x11223344u);
    CHECK_EQ(LDRW(0x02100001u), 0x44112233u);
    CHECK_EQ(LDRW(0x02100002u), 0x33441122u);
    CHECK_EQ(LDRW(0x02100003u), 0x22334411u);
    WR16(0x02100004u, 0xBEEFu);
    WR8(0x02100006u, 0x5Au);
    CHECK_EQ(*(u32 *)DS(0x02100004u), 0x005ABEEFu);
    P32(0x02100008u) = 7;
    CHECK_EQ(RD32(0x02100008u), 7);
    /* registres : par nds_io_read/nds_io_write, pas par la memoire */
    io_reads = io_writes = 0;
    CHECK_EQ(RD32(0x04000130u), 0xC0DE0130u);
    CHECK_EQ(RD16(0x04000004u), 0xC0DE0004u);
    WR16(0x04000208u, 1);
    CHECK_EQ(io_reads, 2);
    CHECK_EQ(io_writes, 1);
    CHECK_EQ(io_last_addr, 0x04000208u);
    CHECK_EQ(io_last_value, 1);
    /* meme chose par le code traduit */
    CHECK_EQ((u32)C1(t_ldr, 0x02100001u), 0x44112233u);
    CHECK_EQ((u32)C1(t_ldr, 0x02100003u), 0x22334411u);
    CHECK_EQ((u32)C1(t_ldr, 0x04000130u), 0xC0DE0130u);
    C2(t_store, 0x02100011u, 0xAABBCCDDu);
    CHECK_EQ(*(u32 *)DS(0x02100010u), 0xAABBCCDDu);
    CHECK_EQ(*(u16 *)DS(0x02100016u), 0xCCDDu);
    CHECK_EQ(*DS(0x0210001Au), 0xDDu);
}

static void test_flag_macros(void) {
    u32 n = 0, z = 0, c = 0, v = 0, res = 0;
#define NZCV() (n << 3 | z << 2 | c << 1 | v)
    SUBF(1u, 2u, 1); CHECK_EQ(res, 0xFFFFFFFFu); CHECK_EQ(NZCV(), 8);
    SUBF(2u, 2u, 1); CHECK_EQ(NZCV(), 6);
    SUBF(0x80000000u, 1u, 1); CHECK_EQ(NZCV(), 3);
    SUBF(0u, 0x80000000u, 1); CHECK_EQ(NZCV(), 9);
    ADDF(0xFFFFFFFFu, 1u, 0); CHECK_EQ(NZCV(), 6);
    ADDF(0x7FFFFFFFu, 1u, 0); CHECK_EQ(NZCV(), 9);
    c = 1; ADDF(1u, 1u, c); CHECK_EQ(res, 3); CHECK_EQ(NZCV(), 0);    /* adc */
    c = 0; SUBF(5u, 3u, c); CHECK_EQ(res, 1); CHECK_EQ(c, 1);          /* sbc : 5 - 3 - 1 */
    NZ(0u); CHECK_EQ(n << 1 | z, 1);
    NZ(0x80000000u); CHECK_EQ(n << 1 | z, 2);
    c = 0; SC_LSL(0x80000001u, 1); CHECK_EQ(c, 1);
    c = 1; SC_LSL(0u, 0); CHECK_EQ(c, 1);                              /* decalage nul : C ne change pas */
    c = 0; SC_LSL(1u, 32); CHECK_EQ(c, 1);
    c = 1; SC_LSL(1u, 33); CHECK_EQ(c, 0);
    c = 0; SC_LSR(3u, 1); CHECK_EQ(c, 1);
    c = 0; SC_LSR(0x80000000u, 32); CHECK_EQ(c, 1);
    c = 0; SC_ASR(0x80000000u, 40); CHECK_EQ(c, 1);
    c = 0; SC_ROR(2u, 2); CHECK_EQ(c, 1);
    c = 0; SC_ROR(0x80000000u, 32); CHECK_EQ(c, 1);
    CHECK_EQ(LSL(1u, 31), 0x80000000u);
    CHECK_EQ(LSL(1u, 32), 0);
    CHECK_EQ(LSL(1u, 256), 1);                                         /* registre : 8 bits bas */
    CHECK_EQ(LSR(0x80000000u, 32), 0);
    CHECK_EQ(ASR(0x80000000u, 40), 0xFFFFFFFFu);
    CHECK_EQ(ROR(0x12345678u, 8), 0x78123456u);
    CHECK_EQ(ROR(0x12345678u, 32), 0x12345678u);
    CHECK_EQ(CLZ(0), 32);
    CHECK_EQ(CLZ(1), 31);
    CHECK_EQ((u32)SAT32(0x80000000LL), 0x7FFFFFFFu);
    CHECK_EQ((u32)SAT32(-0x80000001LL), 0x80000000u);
    n = 1; z = 0; c = 1; v = 0; SAVEF(); n = z = c = v = 0; LOADF(); CHECK_EQ(NZCV(), 10);
#undef NZCV
}

static void test_flags_lifted(void) {
    CHECK_EQ((u32)C2(t_cmpflags, 1, 2), 8);
    CHECK_EQ((u32)C2(t_cmpflags, 2, 2), 6);
    CHECK_EQ((u32)C2(t_cmpflags, 0x80000000u, 1), 3);
    CHECK_EQ((u32)C2(t_cmpflags, 0, 0x80000000u), 9);
    CHECK_EQ((u32)C2(t_cmpflags, 5, 3), 2);
    CHECK_EQ((u32)C2(t_addflags, 0xFFFFFFFFu, 1), 6);
    CHECK_EQ((u32)C2(t_addflags, 0x7FFFFFFFu, 1), 9);
    CHECK_EQ((u32)C2(t_addflags, 1, 1), 0);
    CHECK_EQ((u32)C2(t_addflags, 0x80000000u, 0x80000000u), 7);
    CHECK_EQ((u32)C1(t_lsrs, 1), 6);
    CHECK_EQ((u32)C1(t_lsrs, 2), 0);
    CHECK_EQ((u32)C1(t_lsrs, 3), 2);
    CHECK_EQ((u32)C2(t_smax, 0xFFFFFFFFu, 1), 1);
    CHECK_EQ((u32)C2(t_smax, 5, 0xFFFFFFFDu), 5);
    CHECK_EQ((u32)C2(t_smax, 0x80000000u, 0x7FFFFFFFu), 0x7FFFFFFFu);
}

static void test_callf(void) {
    CHECK_EQ(t_pack10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), PACK10);
    CHECK_EQ(h_pack10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10), PACK10);
    {   /* CALLF tel que le code traduit l'ecrit : r0..r3, puis 10 mots sous sp ; resultat dans r0:r1 */
        u32 r0, r1, r2, r3, sp = 0x02200000u, i;
        for (i = 0; i < 10; i++) P32(sp + 4 * i) = 5 + i;
        r0 = 1; r1 = 2; r2 = 3; r3 = 4;
        CALLF(h_pack10);
        CHECK_EQ(r0, 0x87654321u); CHECK_EQ(r1, 0xA9u);
        r0 = 1; r1 = 2;
        CALLF(t_pack10);
        CHECK_EQ(r0, 0x87654321u); CHECK_EQ(r1, 0xA9u);
        r0 = 1; r1 = 2;
        CALLF(nds_call_target(A_h_pack10));
        CHECK_EQ(r0, 0x87654321u); CHECK_EQ(r1, 0xA9u);
        CALLF(t_args14);                               /* 7e et 10e mots de pile (arguments 11 et 14) */
        CHECK_EQ(r0, 14); CHECK_EQ(r1, 11);
    }
    CHECK_EQ(C1(t_call10, 0), PACK10);                 /* bl d'une fonction traduite */
    host_calls = 0;
    CHECK_EQ(C1(t_callptr, 0), PACK10);                /* pointeur Thumb du pool litteral, blx, fonction C */
    CHECK_EQ(host_calls, 1);
}

static void test_gosub(void) {
    CHECK_EQ((u32)C1(t_gosub, 1), 15);                 /* 4x + 11 : bx lr, mov pc lr, pop {pc} imbrique */
    CHECK_EQ((u32)C1(t_gosub, 5), 31);
    CHECK_EQ((u32)C1(t_thumb, 1), 13);                 /* Thumb : 4x + 9 */
    CHECK_EQ((u32)C1(t_thumb, 2), 17);
}

/* accroches des mods : HOOK_ENTRY de chaque fonction traduite, nds_hook_install de calltab.c */
int nds_hook_install(u32 addr, void *fn, void **original);
typedef u64 (*HookFn)(u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);
static HookFn smax_a, smax_b, pack_a;
#define HOOK_ARGS u32 r0, u32 r1, u32 r2, u32 r3, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9
#define HOOK_PASS(x, y) x, y, r2, r3, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9
static u64 hook_smax_a(HOOK_ARGS) { return smax_a(HOOK_PASS(r0, r1)) + 100; }
static u64 hook_smax_b(HOOK_ARGS) { return smax_b(HOOK_PASS(r1, r0)) + 1000; }
static u64 hook_pack(HOOK_ARGS) { return pack_a(HOOK_PASS(r0, r1)) + 1; }

/* accroche a une instruction : nds_insn_run de mods.c, ici sans liste (une seule fonction de test) */
static void (*insn_fn)(u32 *regs);
void nds_insn_run(u32 k, u32 *regs) { if (k == 0 && insn_fn) insn_fn(regs); }
static void insn_add7(u32 *regs) { regs[0] += 7; regs[16] |= 1u << 30; }

static void test_hooks(void) {
    extern void *nds_insn_slot[];
    insn_fn = insn_add7;
    nds_insn_slot[0] = (void *)1; /* non nul : NDS_INSN appelle nds_insn_run */
    CHECK_EQ((u32)C2(t_smax, 5, 3), 12);               /* r0 modifie avant bx lr */
    CHECK_EQ((u32)C2(t_smax, 2, 9), 16);
    nds_insn_slot[0] = 0;
    CHECK_EQ((u32)C2(t_smax, 2, 9), 9);
    CHECK_EQ((u32)C2(t_smax, 5, 3), 5);
    CHECK_EQ(nds_hook_install(A_t_smax, (void *)hook_smax_a, (void **)&smax_a), 0);
    CHECK_EQ((u32)C2(t_smax, 5, 3), 105);              /* appel direct */
    {   /* pointeur de code du jeu */
        u32 r0 = 5, r1 = 3, r2 = 0, r3 = 0, sp = 0x02200000u;
        CALLF(nds_call_target(A_t_smax));
        CHECK_EQ(r0, 105);
    }
    CHECK_EQ(nds_hook_install(A_t_smax, (void *)hook_smax_b, (void **)&smax_b), 0);
    CHECK_EQ(smax_b == hook_smax_a, 1);                /* le dernier installe recoit le precedent */
    CHECK_EQ((u32)C2(t_smax, 5, 3), 1105);
    CHECK_EQ(nds_hook_install(A_t_pack10, (void *)hook_pack, (void **)&pack_a), 0);
    CHECK_EQ(C1(t_call10, 0), PACK10 + 1);             /* bl d'une fonction traduite vers une autre */
    CHECK_EQ(nds_hook_install(A_t_smax + 4, (void *)hook_smax_a, 0), -1); /* pas une entree de fonction */
    CHECK_EQ(nds_hook_install(A_h_pack10, (void *)hook_smax_a, 0), -1);   /* fonction de l'hote */
    nds_hook_install(A_t_smax, 0, 0);
    nds_hook_install(A_t_pack10, 0, 0);
    CHECK_EQ((u32)C2(t_smax, 5, 3), 5);
    CHECK_EQ(C1(t_call10, 0), PACK10);
}

static void test_call_target(void) {
    CHECK_EQ(nds_call_target(A_t_gosub), (uptr)t_gosub);
    CHECK_EQ(nds_call_target(A_t_gosub | 1), (uptr)t_gosub);
    CHECK_EQ(A_t_thumb & 1, 1);
    CHECK_EQ(nds_call_target(A_t_thumb), (uptr)t_thumb);
    CHECK_EQ(nds_call_target(A_t_thumb & ~1u), (uptr)t_thumb);
    CHECK_EQ(nds_call_target(A_h_pack10), (uptr)h_pack10);
    CHECK_EQ(nds_call_target(0x01FF8000u), (uptr)h_itcm);    /* ITCM : debut de la table directe */
    CHECK_EQ(nds_call_target(0x027E0100u), (uptr)h_dtcm);    /* DTCM : hors de la table directe, recherche */
    EXPECT_FATAL(nds_call_target(A_t_gosub + 4));            /* milieu d'une fonction */
    EXPECT_FATAL(nds_call_target(0x023FFFFEu));              /* derniere entree de la table directe, vide */
    EXPECT_FATAL(nds_call_target(0xFFFFFFFEu));              /* MAGIC_LR */
    EXPECT_FATAL(nds_call_target(0xFFF00000u));              /* GOSUB_TAG */
    EXPECT_FATAL(nds_call_target(0x10001000u));              /* ancienne adresse x86 : plus acceptee */
    EXPECT_FATAL(nds_call_target(0));
    /* depuis le code traduit : blx rX et bx rX */
    CHECK_EQ((u32)C2(t_callreg, A_t_thumb, 1), 13);
    CHECK_EQ((u32)C2(t_callreg, A_t_gosub, 1), 15);
    CHECK_EQ((u32)C2(t_callreg, 0x01FF8001u, 41), 42);
    CHECK_EQ((u32)C2(t_tail, A_t_thumb, 2), 17);
    EXPECT_FATAL(C2(t_callreg, A_t_gosub + 4, 1));
}

/* Fils verts : bascule sur une pile de la fenetre (nds_run_on_stack, comme nds_start), puis deux fils (nds_ctx_init,
 * comme OS_InitContext) qui se passent la main ROUNDS fois chacun par OS_SaveContext/OS_LoadContext, comme
 * irq_deliver. Chaque fil garde des valeurs vivantes a travers la bascule (registres sauvegardes par l'appele :
 * x19-x28 et d8-d15 sur arm64, rbx et r12-r15 sur x86_64), verifie sa pile, l'alignement et nds_cpsr. */
#define GREEN_STACKS 0x20100000u /* adresse DS, apres la pile du fil de test (0x20000000, 1 Mo) */
#define GREEN_STACK 0x10000u
#define CTX_MAIN 0x02300000u    /* cles des Slot : adresses DS d'OSContext */
#define CTX_A 0x02300100u
#define CTX_B 0x02300200u
enum { ROUNDS = 1000000 };
static u32 green_runs[2], green_bad_stack, green_bad_align, green_bad_cpsr, green_bad_values, green_order_bad;
static volatile u32 green_last; /* dernier fil passe : A et B doivent alterner */
static u32 green_entry_arg[2];

static int on_stack(const volatile void *p, u32 lo) {
    u32 d = HOST2DS(p);
    return d >= lo && d < lo + GREEN_STACK;
}

/* cadre aligne sur 16 (x29 sur arm64, rbp sur x86_64) : la pile l'etait a l'appel */
__attribute__((noinline)) static int frame_aligned(void) {
#if defined(__i386__)
    return 1; /* Windows 32 bits : 4 octets suffisent */
#else
    return ((uptr)__builtin_frame_address(0) & 15) == 0;
#endif
}

static void green(u32 self) {
    int me = self == CTX_B;
    u32 other = me ? CTX_A : CTX_B;
    u32 cpsr = me ? 0x1F : 0x13;
    u32 lo = GREEN_STACKS + GREEN_STACK * (1 + me);
    volatile u32 marker = self;
    green_entry_arg[me] = self;
    /* assez de valeurs pour remplir les registres sauvegardes par l'appele */
    u64 k0 = self, k1 = self * 3ull, k2 = self * 5ull, k3 = self * 7ull, k4 = self * 11ull;
    u64 k5 = self * 13ull, k6 = self * 17ull, k7 = self * 19ull, k8 = self * 23ull, k9 = self * 29ull;
    double f0 = me + 0.5, f1 = me + 1.25, f2 = me + 2.125, f3 = me + 3.0625;
    double f4 = me + 4.5, f5 = me + 5.25, f6 = me + 6.125, f7 = me + 7.0625;
    for (u32 i = 0; i < ROUNDS; i++) {
        nds_cpsr = cpsr;
        green_last = self;
        if ((u32)OS_SaveContext(self) == 0) OS_LoadContext(other);
        if (green_last != other) green_order_bad++;
        green_last = self;
        if (nds_cpsr != cpsr) green_bad_cpsr++;
        if (!on_stack(&marker, lo) || marker != self) green_bad_stack++;
        if (!(i & 0xFFF) && !frame_aligned()) green_bad_align++;
        k0 += 1; k1 += 3; k2 += 5; k3 += 7; k4 += 11; k5 += 13; k6 += 17; k7 += 19; k8 += 23; k9 += 29;
        f0 += 1.0; f1 += 1.0; f2 += 1.0; f3 += 1.0; f4 += 1.0; f5 += 1.0; f6 += 1.0; f7 += 1.0;
        green_runs[me]++;
    }
    {
        u64 r = ROUNDS;
        if (k0 != self + r || k1 != (self + r) * 3 || k2 != (self + r) * 5 || k3 != (self + r) * 7 ||
            k4 != (self + r) * 11 || k5 != (self + r) * 13 || k6 != (self + r) * 17 || k7 != (self + r) * 19 ||
            k8 != (self + r) * 23 || k9 != (self + r) * 29)
            green_bad_values++;
        if (f0 != me + 0.5 + r || f1 != me + 1.25 + r || f2 != me + 2.125 + r || f3 != me + 3.0625 + r ||
            f4 != me + 4.5 + r || f5 != me + 5.25 + r || f6 != me + 6.125 + r || f7 != me + 7.0625 + r)
            green_bad_values++;
    }
    /* A finit le premier et laisse B finir son dernier tour ; B rend la main au fil de test */
    OS_LoadContext(me ? CTX_MAIN : CTX_B);
}

static u32 green_start_on_stack;
static void green_start(void) {
    volatile u32 marker = 0;
    green_start_on_stack = on_stack(&marker, GREEN_STACKS) && frame_aligned();
    nds_ctx_init(CTX_A, DS(GREEN_STACKS + GREEN_STACK), GREEN_STACK, green, 0x02000100u);
    nds_ctx_init(CTX_B, DS(GREEN_STACKS + 2 * GREEN_STACK), GREEN_STACK, green, 0x02000200u);
    OS_LoadContext(CTX_A);
}

static void test_context(void) {
    volatile u32 resumed = 0; /* survit a OS_LoadContext(CTX_MAIN) : en memoire, pas dans un registre */
    nds_cpsr = 0x1F;
    if ((u32)OS_SaveContext(CTX_MAIN) == 0) {
        resumed = 1;
        nds_run_on_stack(DS(GREEN_STACKS + GREEN_STACK - 64), green_start);
        CHECK_EQ(0, 1); /* nds_run_on_stack ne revient pas */
    }
    CHECK_EQ(resumed, 1);
    CHECK_EQ(nds_cpsr, 0x1F);
    CHECK_EQ(green_start_on_stack, 1);
    CHECK_EQ(green_entry_arg[0], CTX_A);
    CHECK_EQ(green_entry_arg[1], CTX_B);
    CHECK_EQ(green_runs[0], ROUNDS);
    CHECK_EQ(green_runs[1], ROUNDS);
    CHECK_EQ(green_order_bad, 0);
    CHECK_EQ(green_bad_cpsr, 0);
    CHECK_EQ(green_bad_stack, 0);
    CHECK_EQ(green_bad_align, 0);
    CHECK_EQ(green_bad_values, 0);
}

static void *run_tests(void *arg) {
    nds_code_map_init();
    test_memory();
    test_flag_macros();
    test_flags_lifted();
    test_callf();
    test_gosub();
    test_call_target();
    test_hooks();
    test_context();
    return arg;
}

/* ------------------------------------------------------------------ memoire DS */
#if NDS_IDENTITY
__declspec(dllimport) void *__stdcall VirtualAlloc(void *addr, unsigned long size, unsigned long type, unsigned long prot);

/* lien sans CRT (link_win32 de arm2c_test.py) : exit vide le tampon de printf */
int main(void);
void __main(void) {}
void harness_start(void) { exit(main()); }

int main(void) {
    void *ram = (void *)0x02000000u;
    if (VirtualAlloc(ram, 0x400000, 0x3000 /* MEM_COMMIT | MEM_RESERVE */, 0x04 /* PAGE_READWRITE */) != ram) {
        printf("RAM DS impossible a 0x02000000\n");
        return 2;
    }
    void *green = (void *)GREEN_STACKS;
    if (VirtualAlloc(green, 3 * GREEN_STACK, 0x3000, 0x04) != green) {
        printf("piles des fils verts impossibles\n");
        return 2;
    }
    run_tests(0);
#else
#include <pthread.h>
#include <sys/mman.h>

enum { STACK = 1 << 20 };

int main(void) {
    /* comme os_mem_window : 4 Go + 64 Ko sans acces, puis la RAM principale et une pile a 0x20000000 */
    void *w = mmap(0, (1ull << 32) + 0x10000, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
    if (w == MAP_FAILED) { printf("fenetre DS impossible\n"); return 2; }
    nds_mem = (uptr)w;
    if (mprotect(DS(0x02000000u), 0x400000, PROT_READ | PROT_WRITE) || mprotect(DS(0x20000000u), STACK, PROT_READ | PROT_WRITE) ||
        mprotect(DS(GREEN_STACKS), 3 * GREEN_STACK, PROT_READ | PROT_WRITE)) {
        printf("memoire DS impossible\n");
        return 2;
    }
    /* le code traduit prend sp = HOST2DS(&stack_[n]) : il tourne sur une pile dans la fenetre */
    pthread_attr_t attr;
    pthread_t t;
    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, DS(0x20000000u), STACK);
    if (pthread_create(&t, &attr, run_tests, 0)) { printf("fil de test impossible\n"); return 2; }
    pthread_join(t, 0);
#endif
    printf("%s : %d verifications, %d echecs\n",
#ifdef NDS_INSTRUMENT
           "harnais (NDS_INSTRUMENT)",
#else
           "harnais",
#endif
           checks, failures);
    return failures != 0;
}
