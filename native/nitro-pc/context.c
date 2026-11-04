/* Fils verts du SDK : OS_SaveContext / OS_LoadContext, premier lancement d'un fil, bascule sur la grande pile au demarrage.
 * Registres de l'hote sauvegardes par l'appele, a cote de l'OSContext du SDK (native/MACOS.md 3.3) :
 *   i386 (Windows)        : ebx, esi, edi, ebp, esp, eip
 *   x86_64 (System V)     : rbx, rbp, r12-r15, rsp, rip
 *   x86_64 (Windows)      : les memes, plus rdi, rsi et xmm6-xmm15
 *   arm64 (ABI d'Apple)   : x19-x29, x30 (lr), sp, d8-d15 ; x18 est reserve au systeme, on n'y touche pas
 * Sans en-tete systeme : le test sans rom (native/tests) compile ce fichier tel quel. */
#include "nitro.h"

/* nom d'un symbole C dans l'assembleur : "_" devant sur Mach-O et Windows 32 bits, rien en ELF */
#define STR2(x) #x
#define STR(x) STR2(x)
#define SYM(n) STR(__USER_LABEL_PREFIX__) #n
#define FUNC(n) ".globl " SYM(n) "\n.p2align 4\n" SYM(n) ":\n"

/* ctx : adresse DS de l'OSContext (cle du Slot) ; func : adresse ARM de la fonction du fil (nds_ctx_func) ;
 * sp, pc et le reste : registres de l'hote. Les decalages sont ceux de l'assembleur ci-dessous (verifies plus bas). */
#if defined(__i386__)
typedef struct {
    u32 ctx;
    uptr sp;
    u32 cpsr, func, fresh;
    uptr ebp, ebx, esi, edi, pc;
} Slot;
#define SLOT_SP 4
#define SLOT_PC 36
#elif defined(__x86_64__) && defined(_WIN32)
typedef struct {
    u32 ctx, cpsr, func, fresh;
    uptr sp, pc;
    uptr rbx, rbp, r12, r13, r14, r15, rdi, rsi;
    u64 xmm[20]; /* xmm6..xmm15 */
} Slot;
#define SLOT_SP 16
#define SLOT_PC 24
_Static_assert(__builtin_offsetof(Slot, rsi) == 88 && __builtin_offsetof(Slot, xmm) == 96, "decalages du Slot Windows x64");
#elif defined(__x86_64__)
typedef struct {
    u32 ctx, cpsr, func, fresh;
    uptr sp, pc;
    uptr rbx, rbp, r12, r13, r14, r15;
} Slot;
#define SLOT_SP 16
#define SLOT_PC 24
_Static_assert(__builtin_offsetof(Slot, r15) == 72, "decalages du Slot x86_64");
#elif defined(__aarch64__)
typedef struct {
    u32 ctx, cpsr, func, fresh;
    uptr sp, pc;   /* pc : x30 */
    uptr x[11];    /* x19..x29 */
    u64 d[8];      /* d8..d15 (64 bits bas, les seuls que l'appele garde) */
} Slot;
#define SLOT_SP 16
#define SLOT_PC 24
_Static_assert(__builtin_offsetof(Slot, x) == 32 && __builtin_offsetof(Slot, d) == 120 && sizeof(Slot) == 184,
               "decalages du Slot arm64");
#else
#error "nitro-pc : architecture sans fils verts (i386, x86_64, arm64)"
#endif
_Static_assert(__builtin_offsetof(Slot, sp) == SLOT_SP && __builtin_offsetof(Slot, pc) == SLOT_PC, "decalages du Slot");

enum { SLOTS = 256 }; /* les fils, et les fonctions du jeu avec un catch en cours */
static Slot slots[SLOTS];
/* bornes de la pile de chaque fil, pour le systeme (os_set_stack_bounds) : donnees a sa creation, ou relevees a sa premiere sauvegarde */
static struct { uptr lo, hi; } bounds[SLOTS];
uptr nds_last_load_sp; /* pile du dernier contexte charge, pour le journal d'un plantage */

static Slot *slot_of(u32 ctx) {
    for (int i = 0; i < SLOTS; i++)
        if (slots[i].ctx == ctx) return &slots[i];
    for (int i = 0; i < SLOTS; i++)
        if (!slots[i].ctx) { slots[i].ctx = ctx; return &slots[i]; }
    nds_fatal("trop de threads");
    return 0;
}

/* appelees par l'assembleur, avant de sauver ou de charger les registres */
Slot *nds_save_slot(u32 ctx) {
    Slot *s = slot_of(ctx);
    if (!bounds[s - slots].hi) os_stack_bounds(&bounds[s - slots].lo, &bounds[s - slots].hi);
    s->cpsr = nds_cpsr;
    s->fresh = 0;
    return s;
}

Slot *nds_load_slot(u32 ctx) {
    Slot *s = slot_of(ctx);
    nds_cpsr = s->cpsr;
    if (!s->sp) nds_fatal("contexte sans sauvegarde %x", ctx);
    nds_last_load_sp = s->sp;
    if (bounds[s - slots].hi) os_set_stack_bounds(bounds[s - slots].lo, bounds[s - slots].hi);
    return s;
}

/* Comme setjmp/longjmp : les registres vont dans le Slot, pas sur la pile que l'appelant reutilise ensuite.
 * OS_SaveContext(ctx) rend 0, puis 1 quand OS_LoadContext(ctx) y revient.
 * nds_ctx_start : premier lancement d'un fil prepare par nds_ctx_init, entry(arg) avec arg et entry dans deux
 * registres sauvegardes. nds_run_on_stack(top, fn) : fn sur la pile qui finit a top, sans retour. */
#if defined(__i386__)
__asm__(
    ".text\n"
    FUNC(OS_SaveContext)
    FUNC(nds_eh_save)
    "  mov 4(%esp), %eax\n  push %eax\n  call " SYM(nds_save_slot) "\n  add $4, %esp\n"
    "  mov %ebp, 20(%eax)\n  mov %ebx, 24(%eax)\n  mov %esi, 28(%eax)\n  mov %edi, 32(%eax)\n"
    "  mov (%esp), %ecx\n  mov %ecx, 36(%eax)\n  lea 4(%esp), %ecx\n  mov %ecx, 4(%eax)\n"
    "  xor %eax, %eax\n  ret\n"
    FUNC(OS_LoadContext)
    "  mov 4(%esp), %eax\n  push %eax\n  call " SYM(nds_load_slot) "\n"
    "  mov 4(%eax), %esp\n  mov 20(%eax), %ebp\n  mov 24(%eax), %ebx\n  mov 28(%eax), %esi\n  mov 32(%eax), %edi\n"
    "  mov 36(%eax), %ecx\n  mov $1, %eax\n  jmp *%ecx\n"
    FUNC(nds_run_on_stack)
    "  mov 8(%esp), %eax\n  mov 4(%esp), %esp\n  call *%eax\n  ud2\n");
#elif defined(__x86_64__) && defined(_WIN32)
/* comme System V, mais argument dans rcx, 32 octets reserves sous chaque call, et rdi, rsi, xmm6-xmm15 a garder */
#define XMM_SAVE(op) op " %xmm6, 96(%rax)\n  " op " %xmm7, 112(%rax)\n  " op " %xmm8, 128(%rax)\n  " op " %xmm9, 144(%rax)\n  " \
    op " %xmm10, 160(%rax)\n  " op " %xmm11, 176(%rax)\n  " op " %xmm12, 192(%rax)\n  " op " %xmm13, 208(%rax)\n  " \
    op " %xmm14, 224(%rax)\n  " op " %xmm15, 240(%rax)\n"
#define XMM_LOAD(op) op " 96(%rax), %xmm6\n  " op " 112(%rax), %xmm7\n  " op " 128(%rax), %xmm8\n  " op " 144(%rax), %xmm9\n  " \
    op " 160(%rax), %xmm10\n  " op " 176(%rax), %xmm11\n  " op " 192(%rax), %xmm12\n  " op " 208(%rax), %xmm13\n  " \
    op " 224(%rax), %xmm14\n  " op " 240(%rax), %xmm15\n"
__asm__(
    ".text\n"
    FUNC(OS_SaveContext)
    FUNC(nds_eh_save)
    "  push %rcx\n  sub $32, %rsp\n  call " SYM(nds_save_slot) "\n  add $32, %rsp\n  pop %rcx\n"
    "  mov (%rsp), %rcx\n  mov %rcx, 24(%rax)\n  lea 8(%rsp), %rcx\n  mov %rcx, 16(%rax)\n"
    "  mov %rbx, 32(%rax)\n  mov %rbp, 40(%rax)\n  mov %r12, 48(%rax)\n  mov %r13, 56(%rax)\n"
    "  mov %r14, 64(%rax)\n  mov %r15, 72(%rax)\n  mov %rdi, 80(%rax)\n  mov %rsi, 88(%rax)\n  "
    XMM_SAVE("movups")
    "  xor %eax, %eax\n  ret\n"
    FUNC(OS_LoadContext)
    "  sub $40, %rsp\n  call " SYM(nds_load_slot) "\n"
    "  mov 16(%rax), %rsp\n  mov 32(%rax), %rbx\n  mov 40(%rax), %rbp\n  mov 48(%rax), %r12\n  mov 56(%rax), %r13\n"
    "  mov 64(%rax), %r14\n  mov 72(%rax), %r15\n  mov 80(%rax), %rdi\n  mov 88(%rax), %rsi\n  "
    XMM_LOAD("movups")
    "  mov 24(%rax), %rcx\n  mov $1, %eax\n  jmp *%rcx\n"
    FUNC(nds_ctx_start)
    "  mov %rbx, %rcx\n  sub $32, %rsp\n  call *%r12\n  ud2\n"
    FUNC(nds_run_on_stack)
    "  mov %rcx, %rsp\n  xor %ebp, %ebp\n  sub $32, %rsp\n  call *%rdx\n  ud2\n");
#elif defined(__x86_64__)
/* a l'entree d'une fonction, rsp = 8 modulo 16 ; un call doit partir de rsp = 0 modulo 16 */
__asm__(
    ".text\n"
    FUNC(OS_SaveContext)
    FUNC(nds_eh_save)
    "  push %rdi\n  call " SYM(nds_save_slot) "\n  pop %rdi\n"
    "  mov (%rsp), %rcx\n  mov %rcx, 24(%rax)\n  lea 8(%rsp), %rcx\n  mov %rcx, 16(%rax)\n"
    "  mov %rbx, 32(%rax)\n  mov %rbp, 40(%rax)\n  mov %r12, 48(%rax)\n  mov %r13, 56(%rax)\n"
    "  mov %r14, 64(%rax)\n  mov %r15, 72(%rax)\n"
    "  xor %eax, %eax\n  ret\n"
    FUNC(OS_LoadContext)
    "  sub $8, %rsp\n  call " SYM(nds_load_slot) "\n"
    "  mov 16(%rax), %rsp\n  mov 32(%rax), %rbx\n  mov 40(%rax), %rbp\n  mov 48(%rax), %r12\n  mov 56(%rax), %r13\n"
    "  mov 64(%rax), %r14\n  mov 72(%rax), %r15\n"
    "  mov 24(%rax), %rcx\n  mov $1, %eax\n  jmp *%rcx\n"
    FUNC(nds_ctx_start)
    "  mov %rbx, %rdi\n  call *%r12\n  ud2\n"
    FUNC(nds_run_on_stack)
    "  mov %rdi, %rsp\n  xor %ebp, %ebp\n  call *%rsi\n  ud2\n");
#elif defined(__aarch64__)
/* sp reste aligne sur 16 ; bl ecrase x30, d'ou la paire x29/x30 poussee autour de l'appel dans OS_SaveContext */
__asm__(
    ".text\n"
    FUNC(OS_SaveContext)
    FUNC(nds_eh_save)
    "  stp x29, x30, [sp, #-16]!\n  mov x29, sp\n  bl " SYM(nds_save_slot) "\n  ldp x29, x30, [sp], #16\n"
    "  mov x1, sp\n  stp x1, x30, [x0, #16]\n"
    "  stp x19, x20, [x0, #32]\n  stp x21, x22, [x0, #48]\n  stp x23, x24, [x0, #64]\n  stp x25, x26, [x0, #80]\n"
    "  stp x27, x28, [x0, #96]\n  str x29, [x0, #112]\n"
    "  stp d8, d9, [x0, #120]\n  stp d10, d11, [x0, #136]\n  stp d12, d13, [x0, #152]\n  stp d14, d15, [x0, #168]\n"
    "  mov x0, #0\n  ret\n"
    FUNC(OS_LoadContext)
    "  bl " SYM(nds_load_slot) "\n"
    "  ldp x1, x30, [x0, #16]\n  mov sp, x1\n"
    "  ldp x19, x20, [x0, #32]\n  ldp x21, x22, [x0, #48]\n  ldp x23, x24, [x0, #64]\n  ldp x25, x26, [x0, #80]\n"
    "  ldp x27, x28, [x0, #96]\n  ldr x29, [x0, #112]\n"
    "  ldp d8, d9, [x0, #120]\n  ldp d10, d11, [x0, #136]\n  ldp d12, d13, [x0, #152]\n  ldp d14, d15, [x0, #168]\n"
    "  mov x0, #1\n  ret\n"
    FUNC(nds_ctx_start)
    "  mov x0, x19\n  blr x20\n  brk #1\n"
    FUNC(nds_run_on_stack)
    "  mov sp, x0\n  mov x29, #0\n  mov x30, #0\n  blr x1\n  brk #1\n");
#endif

void nds_ctx_start(void);

/* fil neuf : le premier OS_LoadContext(ctx) appelle entry(ctx) en haut de [stack, stack + size) */
void nds_ctx_init(u32 ctx, u8 *stack, u32 size, void (*entry)(u32), u32 func) {
    Slot *s = slot_of(ctx);
    u8 *top = (u8 *)(((uptr)(stack + size) - 64) & ~(uptr)15);
    nds_zero((u8 *)s + 4, sizeof(Slot) - 4); /* garde ctx, a la tete des trois versions */
    bounds[s - slots].lo = (uptr)stack;
    bounds[s - slots].hi = (uptr)(stack + size);
#if defined(__i386__)
    u32 *t = (u32 *)top;
    t[1] = ctx; /* argument de entry */
    t[0] = 0;   /* adresse de retour fictive */
    s->sp = (uptr)t;
    s->pc = (uptr)entry;
#elif defined(__x86_64__)
    s->sp = (uptr)top;
    s->pc = (uptr)nds_ctx_start;
    s->rbx = ctx;
    s->r12 = (uptr)entry;
#else
    s->sp = (uptr)top;
    s->pc = (uptr)nds_ctx_start;
    s->x[0] = ctx;          /* x19 */
    s->x[1] = (uptr)entry;  /* x20 */
#endif
    s->func = func;
    s->cpsr = 0x1F;
    s->fresh = 1;
}

u32 nds_ctx_func(u32 ctx) { return slot_of(ctx)->func; }

/* ------------------------------------------------------------------ exceptions C++ du jeu (NDS_EH de arm2c.py)
 * Le derouleur du jeu tourne tel quel : il lit la pile ARM et ses tables, appelle les destructeurs. A la fin, au lieu de
 * sauter a l'adresse du catch (func_020b3594 de Pokemon Dash), il arrive ici : on revient dans la fonction de l'hote qui
 * porte ce catch, enregistree a son entree, cle = sp a l'entree (une pile par fil, donc unique). */
u32 nds_sp = 0x027FF000u, nds_lr = 0xFFFFFFFEu, nds_cs[8], nds_eh_regs[11];
static struct { u32 func, size, key; } eh_frames[SLOTS];

u32 nds_eh_enter(u32 func, u32 size, u32 sp) {
    int free_at = -1;
    for (int i = 0; i < SLOTS; i++) {
        if (eh_frames[i].key == sp && eh_frames[i].func == func) return sp;
        if (!eh_frames[i].key && free_at < 0) free_at = i;
    }
    if (free_at < 0) nds_fatal("trop de fonctions avec un catch en cours");
    eh_frames[free_at].func = func;
    eh_frames[free_at].size = size;
    eh_frames[free_at].key = sp;
    return sp;
}

void nds_eh_leave(u32 key) {
    for (int i = 0; i < SLOTS; i++)
        if (eh_frames[i].key == key) eh_frames[i].key = 0;
    for (int i = 0; i < SLOTS; i++)
        if (slots[i].ctx == key) {
            slots[i].ctx = 0;
            bounds[i].lo = bounds[i].hi = 0;
        }
}

extern u64 OS_LoadContext();

void nds_eh_resume(u32 context, u32 info, u32 pc) {
    const u32 *c = (const u32 *)DS(context);
    for (int i = 0; i < 8; i++) nds_eh_regs[i] = c[(0x2C + i * 4) / 4];
    u32 sp = c[0x5C / 4] - c[0x64 / 4];
    nds_eh_regs[8] = sp;
    nds_eh_regs[9] = c[0x64 / 4];
    nds_eh_regs[10] = pc;
    u32 best = 0;
    for (int i = 0; i < SLOTS; i++) {
        u32 key = eh_frames[i].key;
        if (key > sp && pc - eh_frames[i].func < eh_frames[i].size && (!best || key < best)) best = key;
    }
    if (!best) nds_fatal("exception du jeu : pas de fonction de l'hote pour le catch %x, sp=%x", pc, sp);
    OS_LoadContext(best);
}

void nds_dump_threads(void) {
    for (int i = 0; i < SLOTS; i++) {
        if (!slots[i].ctx || !slots[i].sp) continue;
        u32 state = *(u32 *)DS(slots[i].ctx + 0x64);
        nds_log("  thread ctx=%x etat=%u pc=%8x", slots[i].ctx, state, (u32)slots[i].pc);
        u32 *st = (u32 *)slots[i].sp;
        int k = 0;
        for (int j = 0; j < 60 && k < 8; j++)
            if (st[j] >= 0x10000000u && st[j] < 0x11000000u) { nds_log("    %8x", st[j]); k++; }
    }
}
