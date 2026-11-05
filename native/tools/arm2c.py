"""Traduit une fonction ARM/Thumb du binaire DS en C natif (pas d'emulation : le C est compile en x86).

Sert aux fonctions qui n'ont pas de C : assembleur du SDK, fonctions pas encore matchees.
Chaque fonction devient u64 nom(r0, r1, r2, r3, a0..a9) : les arguments de pile passent en a0.., le retour en r0:r1.
Les adresses de donnees restent numeriques : la memoire du jeu est a nds_mem + adresse DS (DS() du PRELUDE ; 0 sur Windows 32 bits).
La pile C de chaque fonction est dans cette memoire (piles des threads dans la fenetre) : sp = HOST2DS(&stack_[n]).
Les pointeurs de code restent des adresses DS (bit Thumb compris), comme dans la rom : un appel indirect passe par
nds_call_target. Les sous-routines internes (bl dans la fonction) mettent dans lr GOSUB_TAG + k, indice dans gs_.
"""
import re

from capstone import CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_THUMB, Cs
from capstone.arm import (
    ARM_INS_BL, ARM_INS_BLX,
    ARM_CC_AL, ARM_CC_LO as ARM_CC_CC, ARM_CC_HS as ARM_CC_CS, ARM_CC_EQ, ARM_CC_GE, ARM_CC_GT, ARM_CC_HI, ARM_CC_LE,
    ARM_CC_LS, ARM_CC_LT, ARM_CC_MI, ARM_CC_NE, ARM_CC_PL, ARM_CC_VC, ARM_CC_VS, ARM_OP_IMM, ARM_OP_MEM, ARM_OP_REG,
    ARM_OP_CIMM, ARM_OP_PIMM,
    ARM_REG_PC, ARM_REG_LR, ARM_REG_SP, ARM_SFT_ASR, ARM_SFT_ASR_REG, ARM_SFT_LSL, ARM_SFT_LSL_REG, ARM_SFT_LSR,
    ARM_SFT_LSR_REG, ARM_SFT_ROR, ARM_SFT_ROR_REG, ARM_SFT_RRX,
)

COND = {
    ARM_CC_EQ: "z", ARM_CC_NE: "!z", ARM_CC_CS: "c", ARM_CC_CC: "!c", ARM_CC_MI: "n", ARM_CC_PL: "!n",
    ARM_CC_VS: "v", ARM_CC_VC: "!v", ARM_CC_HI: "(c&&!z)", ARM_CC_LS: "(!c||z)", ARM_CC_GE: "(n==v)",
    ARM_CC_LT: "(n!=v)", ARM_CC_GT: "(!z&&n==v)", ARM_CC_LE: "(z||n!=v)",
}
LOGIC = {"and", "orr", "eor", "bic", "mov", "mvn", "tst", "teq"}
ARITH = {"add", "adc", "sub", "sbc", "rsb", "rsc", "cmp", "cmn"}
FLAGGED = LOGIC | ARITH | {"mul", "mla", "umull", "smull", "umlal", "smlal", "lsl", "lsr", "asr", "ror", "neg", "rrx"}
SHIFT_ALIAS = {"lsl": "LSL", "lsr": "LSR", "asr": "ASR", "ror": "ROR"}
STACK_ARGS = 10  # mots de pile passes a chaque appel ; AL7E en lit jusqu'a 9 (fonctions de formatage a la printf)
RET = "return ((u64)r1<<32)|r0;"
GOSUB_TAG = 0xFFF00000  # lr des sous-routines internes : jamais une adresse de retour reelle, different de MAGIC_LR

PRELUDE = """/* Genere par arm2c.py */
typedef unsigned int u32; typedef int s32; typedef unsigned short u16; typedef short s16;
typedef unsigned char u8; typedef signed char s8; typedef unsigned long long u64; typedef long long s64;
typedef __UINTPTR_TYPE__ uptr;
/* memoire DS a nds_mem + adresse DS (nitro.h) ; Windows 32 bits : nds_mem = 0, le code genere ne change pas */
#if defined(_WIN32) && defined(__i386__)
#define nds_mem ((uptr)0)
#define NDS_IDENTITY 1
#else
extern uptr nds_mem;
#define NDS_IDENTITY 0
#endif
#define DS(a) ((u8 *)(nds_mem + (u32)(a)))
#if defined(NDS_INSTRUMENT) && !NDS_IDENTITY
u32 nds_host2ds(const volatile void *p);
#define HOST2DS(p) nds_host2ds(p)
#else
#define HOST2DS(p) ((u32)((uptr)(p) - nds_mem))
#endif
#define MAGIC_LR 0xFFFFFFFEu /* lr a l'entree d'une fonction : retour a l'appelant C */
/* sous-routines internes : lr = GOSUB_TAG + k, k indice de gs_ (labels de retour de la fonction, termine par 0) */
#define GOSUB_TAG 0xFFF00000u
#define GOSUB_RET(x) do { u32 _k = (u32)(x) - GOSUB_TAG; if (_k < sizeof gs_ / sizeof gs_[0] - 1) goto *gs_[_k]; } while (0)
static u32 F_N, F_Z, F_C, F_V; /* drapeaux renvoyes par les comparaisons flottantes du compilateur ARM */
#define SAVEF() (F_N = n, F_Z = z, F_C = c, F_V = v)
#define LOADF() (n = F_N, z = F_Z, c = F_C, v = F_V)
extern u32 nds_pc, nds_r[6]; extern u32 nds_cpsr; void nds_cpsr_changed(void); void nds_unhandled(const char *f, u32 pc);
u32 nds_cp15_read(u32 crn, u32 crm, u32 op2); void nds_cp15_write(u32 crn, u32 crm, u32 op2, u32 v);
void nds_swi(u32 n, u32 *r0, u32 *r1, u32 *r2, u32 *r3); uptr nds_call_target(u32 a);
u32 nds_io_read(u32 a, u32 size); void nds_io_write(u32 a, u32 v, u32 size);
#define P32(a) (*(volatile u32 *)DS(a))
#define IS_IO(a) (((a) >> 24) == 4 || (a) < 0x01000000u || (s32)(a) < 0)
static inline u32 RD32(u32 a) { return IS_IO(a) ? nds_io_read(a, 4) : *(volatile u32 *)DS(a); }
static inline u32 RD16(u32 a) { return IS_IO(a) ? nds_io_read(a, 2) : *(volatile u16 *)DS(a); }
static inline u32 RD8(u32 a) { return IS_IO(a) ? nds_io_read(a, 1) : *(volatile u8 *)DS(a); }
#ifdef NDS_INSTRUMENT
extern u32 nds_watch; void nds_watch_hit(u32 a, u32 v);
#define WATCH(a, v) do { if (((a) & ~3u) == nds_watch) nds_watch_hit(a, v); } while (0)
#else
#define WATCH(a, v)
#endif
static inline void WR32(u32 a, u32 v) { WATCH(a, v); if (IS_IO(a)) nds_io_write(a, v, 4); else *(volatile u32 *)DS(a) = v; }
static inline void WR16(u32 a, u32 v) { WATCH(a, v); if (IS_IO(a)) nds_io_write(a, v, 2); else *(volatile u16 *)DS(a) = (u16)v; }
static inline void WR8(u32 a, u32 v) { WATCH(a, v); if (IS_IO(a)) nds_io_write(a, v, 1); else *(volatile u8 *)DS(a) = (u8)v; }
static inline u32 LSL(u32 x, u32 n) { n &= 255; return n >= 32 ? 0 : x << n; }
static inline u32 LSR(u32 x, u32 n) { n &= 255; return n >= 32 ? 0 : x >> n; }
static inline u32 ASR(u32 x, u32 n) { n &= 255; return (u32)((s32)x >> (n >= 32 ? 31 : n)); }
static inline u32 ROR(u32 x, u32 n) { n &= 31; return n ? (x >> n) | (x << (32 - n)) : x; }
static inline u32 LDRW(u32 a) { return ROR(RD32(a & ~3u), (a & 3) * 8); }
static inline u32 CLZ(u32 x) { return x ? __builtin_clz(x) : 32; }
static inline s32 SAT32(s64 v) { return v > 0x7FFFFFFFLL ? 0x7FFFFFFF : v < -0x80000000LL ? (s32)0x80000000 : (s32)v; }
#define SC_LSL(x, k) (c = (k) == 0 ? c : (k) > 32 ? 0 : (k) == 32 ? (x) & 1 : ((x) >> (32 - (k))) & 1)
#define SC_LSR(x, k) (c = (k) == 0 ? c : (k) > 32 ? 0 : (k) == 32 ? (x) >> 31 : ((x) >> ((k) - 1)) & 1)
#define SC_ASR(x, k) (c = (k) == 0 ? c : (k) >= 32 ? (x) >> 31 : ((u32)((s32)(x) >> ((k) - 1))) & 1)
#define SC_ROR(x, k) (c = (k) == 0 ? c : ((k) & 31) == 0 ? (x) >> 31 : ((x) >> (((k) & 31) - 1)) & 1)
#define ADDF(a, b, ci) do { u32 _a = (a), _b = (b); u64 _r = (u64)_a + _b + (ci); res = (u32)_r; c = (u32)(_r >> 32); \\
    v = ((~(_a ^ _b) & (_a ^ res)) >> 31); n = res >> 31; z = res == 0; } while (0)
#define SUBF(a, b, ci) ADDF((a), ~(u32)(b), (ci))
#define NZ(x) (n = (u32)(x) >> 31, z = (u32)(x) == 0)
/* accroches des mods (Lifter.hook_all) : la fonction du jeu f est une entree en assembleur qui saute a nds_hook_slot[k]
 * si un mod l'a remplie, sinon a f__o, le corps traduit. Memes arguments, rien n'est recopie : un seul saut de plus. */
#define HK_STR2(x) #x
#define HK_STR(x) HK_STR2(x)
#define HK_SYM(n) HK_STR(__USER_LABEL_PREFIX__) #n
#if defined(__i386__)
#define HOOK_ENTRY(f, k) __asm__(".text\\n.globl " HK_SYM(f) "\\n.p2align 4\\n" HK_SYM(f) ":\\n  movl " HK_SYM(nds_hook_slot) "+4*" #k ", %eax\\n  testl %eax, %eax\\n  jne 1f\\n  jmp " HK_SYM(f##__o) "\\n1:  jmp *%eax\\n")
#elif defined(__x86_64__) && defined(_WIN32)
#define HOOK_ENTRY(f, k) __asm__(".text\\n.globl " HK_SYM(f) "\\n.p2align 4\\n" HK_SYM(f) ":\\n  movq " HK_SYM(nds_hook_slot) "+8*" #k "(%rip), %rax\\n  testq %rax, %rax\\n  jne 1f\\n  jmp " HK_SYM(f##__o) "\\n1:  jmp *%rax\\n")
#elif defined(__x86_64__)
#define HOOK_ENTRY(f, k) __asm__(".text\\n.globl " HK_SYM(f) "\\n.p2align 4\\n" HK_SYM(f) ":\\n  movq " HK_SYM(nds_hook_slot) "+8*" #k "(%rip), %r11\\n  testq %r11, %r11\\n  jne 1f\\n  jmp " HK_SYM(f##__o) "\\n1:  jmp *%r11\\n")
#elif defined(__aarch64__)
#define HOOK_ENTRY(f, k) __asm__(".text\\n.globl " HK_SYM(f) "\\n.p2align 2\\n" HK_SYM(f) ":\\n  adrp x16, " HK_SYM(nds_hook_slot) "@PAGE\\n  add x16, x16, " HK_SYM(nds_hook_slot) "@PAGEOFF\\n  mov x17, #" #k "\\n  ldr x16, [x16, x17, lsl #3]\\n  cbnz x16, 1f\\n  b " HK_SYM(f##__o) "\\n1:  br x16\\n")
#endif
/* accroches a une instruction (Lifter.insn_hooks) : registres dans nds_regs, rendus au code apres les rappels des mods */
extern void *nds_insn_slot[]; void nds_insn_run(u32 k, u32 *regs);
#define NDS_INSN(k, at) do { if (nds_insn_slot[k]) { u32 _g[17] = {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, at, \
    (n << 31) | (z << 30) | (c << 29) | (v << 28)}; nds_insn_run(k, _g); r0 = _g[0]; r1 = _g[1]; r2 = _g[2]; r3 = _g[3]; \
    r4 = _g[4]; r5 = _g[5]; r6 = _g[6]; r7 = _g[7]; r8 = _g[8]; r9 = _g[9]; r10 = _g[10]; r11 = _g[11]; r12 = _g[12]; sp = _g[13]; \
    lr = _g[14]; n = _g[16] >> 31; z = _g[16] >> 30 & 1; c = _g[16] >> 29 & 1; v = _g[16] >> 28 & 1; } } while (0)
#ifdef NDS_EH
/* exceptions C++ (arm2c.Lifter.eh_end_catch) : pile ARM continue, vraies adresses de retour et registres r4-r11 de l'appelant, le derouleur du jeu les lit */
extern u32 nds_sp, nds_lr, nds_cs[8], nds_eh_regs[11]; extern u64 nds_eh_resume();
u32 nds_eh_enter(u32 func, u32 size, u32 sp); void nds_eh_leave(u32 key); u32 nds_eh_save(u32 key) __attribute__((returns_twice));
#define CALLF(f) do { nds_sp = sp; nds_lr = lr; nds_cs[0] = r4; nds_cs[1] = r5; nds_cs[2] = r6; nds_cs[3] = r7; nds_cs[4] = r8; nds_cs[5] = r9; nds_cs[6] = r10; nds_cs[7] = r11; u64 _t = ((u64 (*)())(f))(r0, r1, r2, r3); r0 = (u32)_t; r1 = (u32)(_t >> 32); } while (0)
#else
#define CALLF(f) do { u64 _t = ((u64 (*)())(f))(r0, r1, r2, r3, @STACK@); r0 = (u32)_t; r1 = (u32)(_t >> 32); } while (0)
#endif
"""
# "shared_regs" (patches.json) : seulement dans lifted.c des jeux qui le demandent
# "arm_stack" (patches.json) : l'appel ne passe que r0-r3 et sp (nds_sp, defini par le runtime), l'appele lit ses arguments
# sur la vraie pile ARM, comme le mode NDS_EH de Pokemon Dash sans les exceptions. gcc compile ainsi 6 a 8 fois plus vite :
# 14 a 36 arguments a chaque appel coutaient l'essentiel de l'allocation de registres. Ajoute apres PRELUDE, qui ne change pas.
ARM_STACK_PRELUDE = """
extern u32 nds_sp;
#undef CALLF
#define CALLF(f) do { nds_sp = sp; u64 _t = ((u64 (*)())(f))(r0, r1, r2, r3); r0 = (u32)_t; r1 = (u32)(_t >> 32); } while (0)
"""
XR_EXTRA = 1024  # mots de pile en plus sous sp : les fonctions partagees empilent dans le cadre de la premiere appelee
XR_PRELUDE = """
u32 nds_xr[14], nds_xr_in;
#define XR_OUT() (nds_xr[2] = r2, nds_xr[3] = r3, nds_xr[4] = r4, nds_xr[5] = r5, nds_xr[6] = r6, nds_xr[7] = r7, nds_xr[8] = r8, nds_xr[9] = r9, nds_xr[10] = r10, nds_xr[11] = r11, nds_xr[12] = r12, nds_xr[13] = sp)
#define XR_IN() (nds_xr[4] = r4, nds_xr[5] = r5, nds_xr[6] = r6, nds_xr[7] = r7, nds_xr[8] = r8, nds_xr[9] = r9, nds_xr[10] = r10, nds_xr[11] = r11, nds_xr[12] = r12, nds_xr[13] = sp, nds_xr_in = 1)
#define XR_LOAD() (r2 = nds_xr[2], r3 = nds_xr[3], r4 = nds_xr[4], r5 = nds_xr[5], r6 = nds_xr[6], r7 = nds_xr[7], r8 = nds_xr[8], r9 = nds_xr[9], r10 = nds_xr[10], r11 = nds_xr[11], r12 = nds_xr[12], sp = nds_xr[13])
#define CALLX(f) do { XR_IN(); CALLF(f); XR_LOAD(); } while (0)
#define CALLXI(a) do { u32 _xa = (a); if (XR_SHARED(_xa)) CALLX(nds_call_target(_xa)); else CALLF(nds_call_target(_xa)); } while (0)
"""


def xr_shared_macro(ranges):
    """XR_SHARED(a) : l'adresse a est-elle dans une plage "shared_regs" ?"""
    return "#define XR_SHARED(a) (%s)\n" % (" || ".join("((u32)(a) - 0x%xu) < 0x%xu" % (lo, hi - lo) for lo, hi in ranges) or "0")

PRELUDE_TEMPLATE = PRELUDE
PRELUDE = PRELUDE.replace("@STACK@", ", ".join("P32(sp + %d)" % (4 * k) for k in range(STACK_ARGS)))


def set_stack_args(n):
    """"stack_args" (patches.json) : plus de mots de pile passes a chaque appel, pour un jeu qui passe de grosses structures par valeur."""
    global STACK_ARGS, PRELUDE
    STACK_ARGS = n
    PRELUDE = PRELUDE_TEMPLATE.replace("@STACK@", ", ".join("P32(sp + %d)" % (4 * k) for k in range(STACK_ARGS)))


def reg_name(i):
    return i.reg_name(i).lower()


class Lifter:
    """symbols : adresse -> nom C ; code_ptr : adresses du binaire qui portent un pointeur de fonction (relocs)."""

    def __init__(self, memory, symbols, thumb, code_ptr, extra_call=None):
        self.memory = memory
        self.symbols = symbols
        self.thumb = thumb
        self.code_ptr = code_ptr
        self.arm = Cs(CS_ARCH_ARM, CS_MODE_ARM)
        self.th = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
        self.arm.detail = self.th.detail = True
        self.new_subs = {}
        self.arm_stack = False  # "arm_stack" (patches.json) : sp = nds_sp, la vraie pile ARM ; pas de cadre stack_ ni de copie d'arguments (ARM_STACK_PRELUDE)
        self.shared_ranges = []  # [(debut, fin)] : assembleur ecrit a la main qui se passe r2-r12 d'une fonction a l'autre (XR_PRELUDE)
        self.sub_prefix = ""  # sous-routines d'un overlay ("overlays": "multi") : ovNNN_sub_...
        self.hooks = {}  # adresse -> fonction du runtime appelee a l'entree, avec r0
        self.exit_hooks = {}  # adresse -> fonction du runtime appelee a chaque sortie
        self.gates = {}  # adresse -> fonction du runtime appelee a l'entree, avec r0 : si elle ne renvoie pas 0, la fonction du jeu ne tourne pas
        self.unhandled = []
        self.flag_funcs = set()  # fonctions dont l'appelant lit les drapeaux au retour
        self.flag_pass = True
        self.gosub_funcs = set()  # fonctions avec des sous-routines internes (bl dans la fonction, registres partages)
        self.sizes = {}
        self.debug_pc = False
        self.eh_end_catch = None  # adresse de __end_catch : le jeu lance de vraies exceptions C++ (NDS_EH du PRELUDE)
        self.hook_all = False  # chaque fonction traduite passe par HOOK_ENTRY : un mod peut la remplacer
        self.hooked = {}  # (adresse, nom) -> (nom, k) : nds_hook_slot[k] de la fonction (deux overlays peuvent avoir la meme adresse)
        self.insn_hooks = {}  # adresse d'instruction -> k : NDS_INSN avant elle, nds_insn_slot[k] ("instruction_hooks" des mods)

    def decode(self, addr, thumb):
        raw = self.memory(addr, 8)
        if raw is None:
            return None
        return next((self.th if thumb else self.arm).disasm(raw, addr, 1), None)

    def call_name(self, addr):
        addr &= ~1
        if addr in self.symbols:
            return self.symbols[addr]
        name = self.new_subs.get(addr)
        if name is None:
            name = "%ssub_%08x" % (self.sub_prefix, addr)
            self.new_subs[addr] = name
        return name

    def is_entry(self, addr):
        return (addr & ~1) in self.symbols or (addr & ~1) in self.new_subs

    # ------------------------------------------------------------ operands
    def rv(self, ins, r):
        name = ins.reg_name(r)
        if r == ARM_REG_PC:
            return "0x%xu" % (ins.address + (4 if self.cur_thumb else 8))
        return {"ip": "r12", "fp": "r11", "sb": "r9", "sl": "r10"}.get(name, name)

    def shifted(self, ins, op, want_carry):
        """Renvoie (expression, instruction carry a placer avant)."""
        if op.type in (ARM_OP_IMM, ARM_OP_PIMM, ARM_OP_CIMM):
            val = op.imm & 0xFFFFFFFF
            carry = "c = %d;" % (val >> 31) if want_carry and val > 255 else ""
            return "0x%xu" % val, carry
        base = self.rv(ins, op.reg)
        st, sv = op.shift.type, op.shift.value
        if st == 0:
            return base, ""
        if st == ARM_SFT_RRX:
            carry = "{u32 _o=c; c=%s&1; sh=(%s>>1)|(_o<<31);}" % (base, base)
            return "sh", carry
        kind = {ARM_SFT_LSL: "LSL", ARM_SFT_LSR: "LSR", ARM_SFT_ASR: "ASR", ARM_SFT_ROR: "ROR",
                ARM_SFT_LSL_REG: "LSL", ARM_SFT_LSR_REG: "LSR", ARM_SFT_ASR_REG: "ASR", ARM_SFT_ROR_REG: "ROR"}[st]
        amount = str(sv) if st in (ARM_SFT_LSL, ARM_SFT_LSR, ARM_SFT_ASR, ARM_SFT_ROR) else "(%s&255)" % self.rv(ins, sv)
        carry = "SC_%s(%s, %s);" % (kind, base, amount) if want_carry else ""
        return "%s(%s,%s)" % (kind, base, amount), carry

    def address(self, ins, mem_op):
        m = mem_op.mem
        base = self.rv(ins, m.base)
        if m.base == ARM_REG_PC:
            base = "0x%xu" % ((ins.address + (4 if self.cur_thumb else 8)) & ~3)
        if m.index:
            idx = self.rv(ins, m.index)
            if mem_op.shift.type:
                kind = {ARM_SFT_LSL: "LSL", ARM_SFT_LSR: "LSR", ARM_SFT_ASR: "ASR", ARM_SFT_ROR: "ROR"}.get(mem_op.shift.type)
                if kind:
                    idx = "%s(%s,%d)" % (kind, idx, mem_op.shift.value)
                elif mem_op.shift.type == ARM_SFT_RRX:
                    idx = "((%s>>1)|(c<<31))" % idx
            off = "-" if mem_op.subtracted else "+"
            return "%s%s%s" % (base, off, idx), base
        disp = m.disp
        return ("%s%+d" % (base, disp) if disp else base), base

    # ------------------------------------------------------------ one instruction
    def emit(self, ins, labels):
        """Renvoie (texte C, fin de bloc). labels sert aux branches."""
        ops = ins.operands
        mn = ins.mnemonic.split(".")[0]
        cc = ins.cc if ins.cc not in (ARM_CC_AL, 0) else None
        names = ["eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "cs", "cc"]
        if cc is not None:
            for nm in names:
                if mn.endswith(nm):
                    mn = mn[:-2]
                    break
        setf = False
        if mn.endswith("s") and mn[:-1] in FLAGGED:
            setf, mn = True, mn[:-1]
        if mn in ("cmp", "cmn", "tst", "teq"):
            setf = True
        body, ends = self.body(ins, mn, setf, ops, labels)
        if body is None:
            self.unhandled.append("%08x %s %s" % (ins.address, ins.mnemonic, ins.op_str))
            body = 'nds_unhandled(__func__, 0x%x);' % ins.address
        if cc is not None:
            return "if (%s) { %s }" % (COND[cc], body), False
        return body, ends

    def body(self, ins, mn, setf, ops, labels):
        rv = lambda o: self.rv(ins, o.reg)
        if mn in ("nop", "pld"):
            return ";", False
        if mn == "adr":
            base = (ins.address + (4 if self.cur_thumb else 8)) & ~3
            return "%s = 0x%xu;" % (rv(ops[0]), base + ops[1].imm), False
        if mn in ("b", "bl", "blx", "bx") or (mn == "cbz"):
            return self.branch(ins, mn, ops, labels)
        # ecriture de pc par une operation de donnees
        if mn in ("mov", "add") and ops and ops[0].type == ARM_OP_REG and ops[0].reg == ARM_REG_PC:
            return self.pc_write(ins, mn, ops, labels)
        if mn in ("sub", "rsb", "and", "orr", "eor", "bic", "mvn", "adc", "sbc") and ops and ops[0].type == ARM_OP_REG and ops[0].reg == ARM_REG_PC:
            # "subs pc, lr, #4" : fin de gestionnaire d'exception, sans equivalent ici
            return RET, ins.cc in (ARM_CC_AL, 0)
        if mn in ("and", "orr", "eor", "bic", "mov", "mvn", "add", "adc", "sub", "sbc", "rsb", "rsc", "cmp", "cmn", "tst", "teq", "neg"):
            return self.dataop(ins, mn, setf, ops), False
        if mn in ("lsl", "lsr", "asr", "ror"):
            d = rv(ops[0])
            if len(ops) == 3:
                a, b = ops[1], ops[2]
                amt = str(b.imm) if b.type == ARM_OP_IMM else "(%s&255)" % rv(b)
            elif ops[1].shift.type:
                # "lsrs r1, r2, #5" arrive comme r1, (r2 avec decalage)
                a = ops[1]
                st, sv = a.shift.type, a.shift.value
                amt = str(sv) if st in (ARM_SFT_LSL, ARM_SFT_LSR, ARM_SFT_ASR, ARM_SFT_ROR) else "(%s&255)" % self.rv(ins, sv)
            else:
                a, b = ops[0], ops[1]
                amt = str(b.imm) if b.type == ARM_OP_IMM else "(%s&255)" % rv(b)
            k = mn.upper()
            s = "SC_%s(%s, %s); " % (k, rv(a), amt) if setf else ""
            s += "%s = %s(%s,%s);" % (d, k, rv(a), amt)
            return s + (" NZ(%s);" % d if setf else ""), False
        if mn == "rrx":
            d, a = rv(ops[0]), rv(ops[1])
            s = "{u32 _o=c; u32 _x=%s; %s c=_x&1; %s=(_x>>1)|(_o<<31);}" % (a, "", d)
            return s + (" NZ(%s);" % d if setf else ""), False
        if mn == "mul":
            d, a = rv(ops[0]), rv(ops[1])
            b = rv(ops[2]) if len(ops) > 2 else d
            return "%s = %s * %s;" % (d, a, b) + (" NZ(%s);" % d if setf else ""), False
        if mn == "mla":
            d = rv(ops[0])
            return "%s = %s * %s + %s;" % (d, rv(ops[1]), rv(ops[2]), rv(ops[3])) + (" NZ(%s);" % d if setf else ""), False
        if mn in ("umull", "smull", "umlal", "smlal"):
            lo, hi, a, b = (rv(o) for o in ops[:4])
            cast = "(u64)(u32)" if mn[0] == "u" else "(s64)(s32)"
            acc = "((u64)%s<<32 | %s) + " % (hi, lo) if mn.endswith("al") else ""
            s = "{u64 _r = %s(u64)(%s%s * %s%s); %s=(u32)_r; %s=(u32)(_r>>32);}" % (acc, cast, a, cast, b, lo, hi)
            return s + (" n=%s>>31; z=(%s|%s)==0;" % (hi, hi, lo) if setf else ""), False
        m = re.fullmatch(r"(smul|smla)([bt])([bt])", mn)
        if m:
            half = lambda r, w: "(s32)(s16)(%s>>16)" % r if w == "t" else "(s32)(s16)%s" % r
            d = rv(ops[0])
            s = "%s * %s" % (half(rv(ops[1]), m.group(2)), half(rv(ops[2]), m.group(3)))
            return "%s = %s%s;" % (d, s, " + " + rv(ops[3]) if m.group(1) == "smla" else ""), False
        m = re.fullmatch(r"(smulw|smlaw)([bt])", mn)
        if m:
            half = "(s32)(s16)(%s>>16)" % rv(ops[2]) if m.group(2) == "t" else "(s32)(s16)%s" % rv(ops[2])
            d = rv(ops[0])
            s = "(u32)(((s64)(s32)%s * %s) >> 16)" % (rv(ops[1]), half)
            return "%s = %s%s;" % (d, s, " + " + rv(ops[3]) if m.group(1) == "smlaw" else ""), False
        if mn in ("smlalbb", "smlalbt", "smlaltb", "smlaltt"):
            half = lambda r, w: "(s64)(s16)(%s>>16)" % r if w == "t" else "(s64)(s16)%s" % r
            lo, hi = rv(ops[0]), rv(ops[1])
            return ("{u64 _r=((u64)%s<<32|%s)+(u64)(%s*%s); %s=(u32)_r; %s=(u32)(_r>>32);}"
                    % (hi, lo, half(rv(ops[2]), mn[5]), half(rv(ops[3]), mn[6]), lo, hi)), False
        if mn == "clz":
            return "%s = CLZ(%s);" % (rv(ops[0]), rv(ops[1])), False
        if mn in ("qadd", "qsub"):
            sign = "+" if mn == "qadd" else "-"
            return "%s = SAT32((s64)(s32)%s %s (s64)(s32)%s);" % (rv(ops[0]), rv(ops[1]), sign, rv(ops[2])), False
        if mn in ("qdadd", "qdsub"):
            sign = "+" if mn == "qdadd" else "-"
            return "%s = SAT32((s64)(s32)%s %s (s64)SAT32(2*(s64)(s32)%s));" % (rv(ops[0]), rv(ops[1]), sign, rv(ops[2])), False
        if mn in ("ldr", "ldrb", "ldrh", "ldrsb", "ldrsh", "ldrd", "str", "strb", "strh", "strd"):
            return self.loadstore(ins, mn, ops, labels)
        if mn in ("ldm", "ldmia", "ldmib", "ldmda", "ldmdb", "stm", "stmia", "stmib", "stmda", "stmdb", "ldmfd", "stmfd", "push", "pop"):
            return self.multi(ins, mn, ops)
        if mn in ("swp", "swpb"):
            d, s, m = rv(ops[0]), rv(ops[1]), self.address(ins, ops[2])[0]
            w = "8" if mn == "swpb" else "32"
            return "{u32 _a=%s,_v=%s; u32 _o=RD%s(_a); WR%s(_a,_v); %s=_o;}" % (m, s, w, w, d), False
        if mn == "mrs":
            return "%s = nds_cpsr | ((u32)n<<31)|((u32)z<<30)|((u32)c<<29)|((u32)v<<28);" % rv(ops[0]), False
        if mn == "msr":
            src = ops[-1]
            val = "0x%xu" % (src.imm & 0xFFFFFFFF) if src.type in (ARM_OP_IMM, ARM_OP_PIMM) else rv(src)
            parts = ins.op_str.split(",")[0].strip()
            fields = parts.split("_")[1] if "_" in parts else "fc"
            s = "{u32 _v=%s;" % val
            if "f" in fields:
                s += " n=_v>>31; z=(_v>>30)&1; c=(_v>>29)&1; v=(_v>>28)&1;"
            if "c" in fields:
                s += " nds_cpsr=(nds_cpsr&~0xFFu)|(_v&0xFF); nds_cpsr_changed();"
            return s + "}", False
        if mn == "mcr":
            return "nds_cp15_write(%d,%d,%d,%s);" % (ops[3].imm, ops[4].imm, ops[5].imm if len(ops) > 5 else 0, rv(ops[2])), False
        if mn == "mrc":
            return "%s = nds_cp15_read(%d,%d,%d);" % (rv(ops[2]), ops[3].imm, ops[4].imm, ops[5].imm if len(ops) > 5 else 0), False
        if mn in ("svc", "swi"):
            return "nds_swi(%d,&r0,&r1,&r2,&r3);" % (ops[0].imm & 0xFF), False
        if mn in ("bkpt", "udf"):
            return None, False
        return None, False

    def dataop(self, ins, mn, setf, ops):
        rv = lambda o: self.rv(ins, o.reg)
        logic = mn in LOGIC
        cmpish = mn in ("cmp", "cmn", "tst", "teq")
        if mn == "neg":
            d, a = rv(ops[0]), rv(ops[1])
            return ("SUBF(0, %s, 1); %s = res;" % (a, d)) if setf else "%s = 0u - %s;" % (d, a)
        if cmpish:
            d, a, src = None, rv(ops[0]), ops[1]
        elif mn in ("mov", "mvn"):
            d, a, src = rv(ops[0]), None, ops[1]
        else:
            d = rv(ops[0])
            if len(ops) == 3:
                a, src = rv(ops[1]), ops[2]
            else:
                a, src = d, ops[1]
        val, carry = self.shifted(ins, src, setf and logic)
        pre = carry + " " if carry else ""
        if setf and not logic:
            cin = {"add": "0", "cmn": "0", "adc": "c", "sub": "1", "cmp": "1", "sbc": "c", "rsb": "1", "rsc": "c"}[mn]
            if mn in ("add", "adc", "cmn"):
                s = "ADDF(%s, %s, %s);" % (a, val, cin)
            elif mn in ("rsb", "rsc"):
                s = "SUBF(%s, %s, %s);" % (val, a, cin)
            else:
                s = "SUBF(%s, %s, %s);" % (a, val, cin)
            return pre + s + ("" if cmpish else " %s = res;" % d)
        expr = {
            "and": "%s & %s" % (a, val), "tst": "%s & %s" % (a, val), "orr": "%s | %s" % (a, val),
            "eor": "%s ^ %s" % (a, val), "teq": "%s ^ %s" % (a, val), "bic": "%s & ~(%s)" % (a, val),
            "mov": val, "mvn": "~(%s)" % val, "add": "%s + %s" % (a, val), "adc": "%s + %s + c" % (a, val),
            "sub": "%s - %s" % (a, val), "sbc": "%s - %s - !c" % (a, val), "rsb": "%s - %s" % (val, a),
            "rsc": "%s - %s - !c" % (val, a),
        }[mn]
        if cmpish:
            return pre + "res = %s; NZ(res);" % expr
        s = pre + "%s = %s;" % (d, expr)
        if setf:
            s += " NZ(%s);" % d
        return s

    def literal(self, addr):
        """Valeur d'un mot du binaire lu par ldr rX, [pc, #n]."""
        raw = self.memory(addr, 4)
        if raw is None:
            return None
        value = int.from_bytes(raw, "little")
        if addr in self.code_ptr:
            # adresse ARM telle quelle (bit Thumb compris) ; call_name fait traduire la fonction visee
            return "0x%xu /* &%s */" % (value, self.call_name(value))
        return "0x%xu" % value

    def loadstore(self, ins, mn, ops, labels):
        rv = lambda o: self.rv(ins, o.reg)
        data = ops[0]
        mem = next(o for o in ops if o.type == ARM_OP_MEM)
        d = rv(data)
        # table de sauts : ldr pc, [pc, rX, lsl #2]
        if mn == "ldr" and data.reg == ARM_REG_PC:
            return self.pc_load(ins, mem, labels)
        # pool litteral
        if mn == "ldr" and mem.mem.base == ARM_REG_PC and not mem.mem.index:
            addr = ((ins.address + (4 if self.cur_thumb else 8)) & ~3) + mem.mem.disp
            lit = self.literal(addr)
            if lit is not None:
                return "%s = %s;" % (d, lit), False
        addr, base = self.address(ins, mem)
        post = len(ops) >= 3 and ops[-1].type in (ARM_OP_IMM, ARM_OP_REG) and ops[-1] is not mem
        pre_wb = ins.writeback and not post
        s = "{"
        if post:
            s += "u32 _a=%s;" % base
            extra = ops[-1]
            if extra.type == ARM_OP_IMM:
                # "strb r1, [r3], #-1" : capstone donne 1 avec le drapeau subtracted
                off = "%s%+d" % (base, -abs(extra.imm) if extra.subtracted else extra.imm)
            else:
                idx = rv(extra)
                if extra.shift.type:
                    kind = {ARM_SFT_LSL: "LSL", ARM_SFT_LSR: "LSR", ARM_SFT_ASR: "ASR", ARM_SFT_ROR: "ROR"}[extra.shift.type]
                    idx = "%s(%s,%d)" % (kind, idx, extra.shift.value)
                off = "%s%s%s" % (base, "-" if extra.subtracted else "+", idx)
        else:
            s += "u32 _a=%s;" % addr
        wb = ""
        if post:
            wb = "%s=%s;" % (base, off)
        elif pre_wb:
            wb = "%s=_a;" % base
        if mn == "ldrd":
            d2 = rv(ops[1])
            return s + "{u32 _x=RD32(_a&~3u),_y=RD32((_a&~3u)+4); %s %s=_x; %s=_y;}}" % (wb, d, d2), False
        if mn == "strd":
            d2 = rv(ops[1])
            return s + "WR32(_a&~3u,%s); WR32((_a&~3u)+4,%s); %s}" % (d, d2, wb), False
        if mn.startswith("ldr"):
            rd = {"ldr": "LDRW(_a)", "ldrb": "RD8(_a)", "ldrh": "RD16(_a&~1u)", "ldrsb": "(s32)(s8)RD8(_a)", "ldrsh": "(s32)(s16)RD16(_a&~1u)"}[mn]
            return s + "{u32 _x=%s; %s %s=_x;}}" % (rd, wb, d), False
        wr = {"str": "WR32(_a&~3u,%s);", "strb": "WR8(_a,%s);", "strh": "WR16(_a&~1u,%s);"}[mn] % d
        return s + wr + " " + wb + "}", False

    def multi(self, ins, mn, ops):
        rv = lambda o: self.rv(ins, o.reg)
        if mn in ("push", "pop"):
            base = "sp"
            regs = list(ops)
            load, mode = (mn == "pop"), ("ia" if mn == "pop" else "db")
            wb = True
        else:
            base = rv(ops[0])
            regs = list(ops[1:])
            load = mn.startswith("ldm")
            mode = {"fd": "ia" if load else "db"}.get(mn[3:], mn[3:] or "ia")
            wb = ins.writeback
        n = len(regs)
        start = {"ia": "_a", "ib": "_a+4", "da": "_a-%d" % (4 * (n - 1)), "db": "_a-%d" % (4 * n)}[mode]
        end = {"ia": "_a+%d" % (4 * n), "ib": "_a+%d" % (4 * n), "da": "_a-%d" % (4 * n), "db": "_a-%d" % (4 * n)}[mode]
        s = "{u32 _a=%s&~3u; u32 _p=%s;" % (base, start)
        returns = False
        if load:
            base_loaded = False
            for r in regs:
                if r.reg == ARM_REG_PC:
                    returns = True
                    s += "pc_val=RD32(_p);_p+=4;" if self.cur_name in self.gosub_funcs else "_p+=4;"
                else:
                    s += ("%s=P32(_p);_p+=4;" if base == "sp" else "%s=RD32(_p);_p+=4;") % rv(r)
                    if rv(r) == base:
                        base_loaded = True
            if wb and not base_loaded:
                s += "%s=%s;" % (base, end)
        else:
            for r in regs:
                val = "0x%xu" % (ins.address + 8) if r.reg == ARM_REG_PC else rv(r)
                s += ("P32(_p)=%s;_p+=4;" if base == "sp" else "WR32(_p,%s);_p+=4;") % val
            if wb:
                s += "%s=%s;" % (base, end)
        s += "}"
        if returns:
            if self.cur_name in self.gosub_funcs:
                return s + " " + self.gosub_ret("pc_val"), ins.cc in (ARM_CC_AL, 0)
            return s + " " + RET, True
        return s, False

    # ------------------------------------------------------------ control flow
    def gosub_ret(self, reg):
        """Retour calcule d'une fonction a sous-routines internes : vers l'appelant (MAGIC_LR), un label de gs_,
        ou sinon une vraie adresse de code (saut terminal par nds_call_target, comme bx rX)."""
        return "if (%s != MAGIC_LR) { GOSUB_RET(%s); CALLF(nds_call_target(%s)); } %s" % (reg, reg, reg, RET)

    def gosub_tag(self, ret_addr):
        if ret_addr not in self.gosub_labels:
            self.gosub_labels.append(ret_addr)
        return GOSUB_TAG + self.gosub_labels.index(ret_addr)

    def branch(self, ins, mn, ops, labels):
        cond = ins.cc not in (ARM_CC_AL, 0)
        if mn == "bl" and ops[0].type in (ARM_OP_IMM, ARM_OP_PIMM):
            target = ops[0].imm & 0xFFFFFFFF
            size = self.sizes.get(self.entry, 0)
            if target not in self.symbols and self.entry <= target < self.entry + size:
                self.gosub_funcs.add(self.cur_name)
                labels.add(target)
                nxt = ins.address + ins.size
                labels.add(nxt)
                return "lr = 0x%xu; goto L%08x;" % (self.gosub_tag(nxt), target), False
        if mn in ("bl", "blx") and ops[0].type in (ARM_OP_IMM, ARM_OP_PIMM):
            name = self.call_name(ops[0].imm)
            after = self.decode(ins.address + ins.size, self.cur_thumb)
            if after is not None and after.cc not in (ARM_CC_AL, 0):
                self.flag_funcs.add(name)
            load = " LOADF();" if name in self.flag_funcs else ""
            return "CALLF(%s);%s" % (name, load), False
        if mn == "blx":
            return "CALLF(nds_call_target(%s));" % self.rv(ins, ops[0].reg), False
        if mn == "bx":
            r = ops[0].reg
            if r == ARM_REG_LR:
                if self.cur_name in self.gosub_funcs:
                    # retour de sous-routine interne, ou de la fonction : on ne le sait qu'a l'execution
                    return self.gosub_ret("lr"), ins.cc in (ARM_CC_AL, 0)
                return RET, True
            name = self.rv(ins, r)
            if self.prev_set_lr:
                return "CALLF(nds_call_target(%s));" % name, False
            return "if (%s==MAGIC_LR) { %s } CALLF(nds_call_target(%s)); %s" % (name, RET, name, RET), True
        target = ops[0].imm & 0xFFFFFFFF
        if self.is_entry(target) and target != self.entry:
            callee = self.call_name(target)
            if self.cur_name in self.flag_funcs:
                self.flag_funcs.add(callee)
            load = " LOADF();" if callee in self.flag_funcs else ""
            return "CALLF(%s);%s %s" % (callee, load, RET), True
        labels.add(target)
        return "goto L%08x;" % target, not cond

    def pc_write(self, ins, mn, ops, labels):
        src = ops[1]
        if mn == "mov" and src.type == ARM_OP_REG:
            if src.reg == ARM_REG_LR:
                if self.cur_name in self.gosub_funcs:
                    return self.gosub_ret("lr"), ins.cc in (ARM_CC_AL, 0)
                return RET, True
            name = self.rv(ins, src.reg)
            if self.prev_set_lr:
                return "CALLF(nds_call_target(%s));" % name, False
            return "if (%s==MAGIC_LR) { %s } CALLF(nds_call_target(%s)); %s" % (name, RET, name, RET), True
        # add pc, pc, rX, lsl #2 suivi d'une table de "b"
        if mn == "add":
            return self.jump_table(ins, ops, labels)
        return None, False

    def table_limit(self, ins):
        """Nombre d'entrees d'apres le cmp qui precede l'aiguillage (dans les quelques instructions d'avant)."""
        for prev in reversed(self.recent[-6:]):
            if prev.mnemonic.startswith("cmp") and prev.operands[1].type == ARM_OP_IMM:
                return prev.operands[0].reg, prev.operands[1].imm + 1
        return None, None

    def jump_table(self, ins, ops, labels):
        idx = ops[-1] if len(ops) >= 2 else None
        reg = idx.reg if idx is not None and idx.type == ARM_OP_REG else None
        creg, count = self.table_limit(ins)
        base = ins.address + (4 if self.cur_thumb else 8)
        if creg != reg or not count or count < 2:
            # le cmp qui borne la table est parfois loin devant : la table s'arrete au premier mot qui n'est pas un "b"
            count = 0
            while count < 512:
                entry = self.decode(base + count * 4, False)
                if entry is None or entry.mnemonic != "b":
                    break
                count += 1
        if not count and not self.cur_thumb:
            # saut au milieu d'une suite d'instructions (dispositif de Duff) : chaque instruction est une entree
            count = 0
            while count < 512:
                entry = self.decode(base + count * 4, False)
                if entry is None:
                    break
                count += 1
                ctl = entry.mnemonic.split(".")[0]
                if ctl in ("b", "bx") or "pc" in entry.op_str.split(",")[0] or ("pop" in ctl and "pc" in entry.op_str):
                    break
        if reg is None or not count or count > 512:
            return None, False
        cases = []
        # idx deja multiplie par 4 si le decalage est absent : cas ARM classique = lsl #2
        for i in range(count):
            target = base + i * 4
            cases.append("case %d: goto L%08x;" % (i, target))
            labels.add(target)
            self.extra_starts.append(target)
        name = self.rv(ins, reg)
        return "switch (%s) { %s default: nds_unhandled(__func__, 0x%x); }" % (name, " ".join(cases), ins.address), True

    def pc_load(self, ins, mem, labels):
        m = mem.mem
        if m.base == ARM_REG_PC and m.index:
            creg, count = self.table_limit(ins)
            if count is None or count > 512:
                return None, False
            base = (ins.address + (4 if self.cur_thumb else 8)) & ~3
            cases = []
            for i in range(count):
                raw = self.memory(base + i * 4, 4)
                target = int.from_bytes(raw, "little") & ~1
                cases.append("case %d: goto L%08x;" % (i, target))
                labels.add(target)
                self.extra_starts.append(target)
            return "switch (%s) { %s default: nds_unhandled(__func__, 0x%x); }" % (self.rv(ins, m.index), " ".join(cases), ins.address), True
        addr = self.address(ins, mem)[0]
        return "{u32 _t=RD32(%s); CALLF(nds_call_target(_t)); %s}" % (addr, RET), True

    # ------------------------------------------------------------ whole function
    def lift(self, name, entry, thumb):
        self.entry = entry
        self.cur_name = name
        self.cur_thumb = thumb
        self.extra_starts = []
        self.recent = []
        self.gosub_labels = []  # retours des sous-routines internes, dans l'ordre des tags
        out = {}
        labels = set()
        seen_order = []
        work = [entry]
        pending_next = {}
        pads = []
        while work:
            while work:
                pc = work.pop()
                self.recent = []
                self.prev_set_lr = False
                while pc not in out:
                    if pc != entry and self.is_entry(pc):
                        out[pc] = ("CALLF(%s); %s" % (self.call_name(pc), RET), None, 0)
                        break
                    ins = self.decode(pc, thumb)
                    if ins is None:
                        out[pc] = ('nds_unhandled(__func__, 0x%x);' % pc, None, 2)
                        break
                    before = set(labels)
                    text, ends = self.emit(ins, labels)
                    if self.eh_end_catch is not None and ins.id in (ARM_INS_BL, ARM_INS_BLX):
                        text = text.replace("CALLF(", "lr=0x%xu;CALLF(" % (pc + ins.size))
                    if self.debug_pc:
                        text = "nds_pc = 0x%x; nds_r[0]=r0; nds_r[1]=r1; nds_r[2]=r2; nds_r[3]=r3; nds_r[4]=r4; nds_r[5]=r5; %s" % (pc, text)
                    if pc in self.insn_hooks:
                        text = "NDS_INSN(%d, 0x%xu); %s" % (self.insn_hooks[pc], pc, text)
                    out[pc] = (text, pc + ins.size, ins.size)
                    self.prev_set_lr = ins.mnemonic.startswith("mov") and ins.op_str.startswith("lr, pc")
                    self.recent.append(ins)
                    work.extend(labels - before)
                    if self.extra_starts:
                        work.extend(self.extra_starts)
                        self.extra_starts = []
                    if ends:
                        break
                    pc += ins.size
            pad = self.landing_pad(entry, out)
            if pad is not None:
                pads.append(pad)
                labels.add(pad)
                work.append(pad)
        lines = []
        addrs = sorted(out)
        for i, a in enumerate(addrs):
            text, nxt, size = out[a]
            if a in labels:
                lines.append("L%08x:;" % a)
            lines.append("  " + text)
            follows = addrs[i + 1] if i + 1 < len(addrs) else None
            ended = text.endswith(RET) or text.endswith("}") and "return" in text and not text.startswith("if")
            if nxt is not None and follows != nxt and nxt in out and not ended:
                lines.append("  goto L%08x;" % nxt)
                labels.add(nxt)
        # les labels vises apres coup doivent etre poses
        final = []
        placed = set()
        for i, a in enumerate(addrs):
            pass
        for line in lines:
            final.append(line)
        text = "\n".join(final)
        for a in sorted(labels):
            label = "L%08x:;" % a
            if label not in text and a in out:
                marker = "  " + out[a][0]
                text = text.replace(marker, label + "\n" + marker, 1)
        args = ", ".join(["u32 r0", "u32 r1", "u32 r2", "u32 r3"] + ["u32 a%d" % i for i in range(STACK_ARGS)])
        xr = any(lo <= entry < hi for lo, hi in self.shared_ranges)
        extra = XR_EXTRA if xr else 0
        head = [
            "u64 %s(%s) {" % (name + "__o" if self.hook_all else name, args),
            "  u32 stack_[%d]; u32 sp = HOST2DS(&stack_[%d]);" % (self.frame_words(out) + STACK_ARGS + 2 + extra, self.frame_words(out) + extra),
            "  u32 r4=0,r5=0,r6=0,r7=0,r8=0,r9=0,r10=0,r11=0,r12=0,lr=MAGIC_LR,res=0,sh=0,pc_val=0; u32 n=0,z=0,c=0,v=0;",
            "  " + " ".join("stack_[%d]=a%d;" % (self.frame_words(out) + extra + i, i) for i in range(STACK_ARGS)),
        ]
        if self.arm_stack and self.eh_end_catch is None:
            head[1] = "  u32 sp = nds_sp;"
            head[3] = ""
        if self.eh_end_catch is not None:
            head[1] = "  u32 sp = nds_sp;"
            head[2] = head[2].replace("lr=MAGIC_LR", "lr=nds_lr")
            head[2] = head[2].replace("r4=0,r5=0,r6=0,r7=0,r8=0,r9=0,r10=0,r11=0,", "r4=nds_cs[0],r5=nds_cs[1],r6=nds_cs[2],r7=nds_cs[3],r8=nds_cs[4],r9=nds_cs[5],r10=nds_cs[6],r11=nds_cs[7],")
            head[3] = "  u32 lr0_ = lr;"
            text = re.sub(r"\((\w+)==MAGIC_LR\)", r"(\1==MAGIC_LR||\1==lr0_)", text)
            text = re.sub(r"\((\w+) != MAGIC_LR\)", r"(\1 != MAGIC_LR\&\&\1 != lr0_)", text)
            if pads:
                cases = " ".join("case 0x%xu: goto L%08x;" % (p, p) for p in pads)
                head.append("  u32 eh_key_ = nds_eh_enter(0x%xu, 0x%xu, sp);" % (entry, self.sizes.get(entry, 0)))
                head.append("  if (nds_eh_save(eh_key_)) { r4=nds_eh_regs[0]; r5=nds_eh_regs[1]; r6=nds_eh_regs[2]; r7=nds_eh_regs[3]; r8=nds_eh_regs[4]; r9=nds_eh_regs[5]; r10=nds_eh_regs[6]; r11=nds_eh_regs[7]; sp=nds_eh_regs[8]; r12=nds_eh_regs[9]; r0=r1=r2=r3=0; switch (nds_eh_regs[10]) { %s default: nds_unhandled(__func__, nds_eh_regs[10]); } }" % cases)
        if name in self.gosub_funcs:
            head.append("  static void *const gs_[] = { %s0 };" % "".join("&&L%08x, " % a for a in self.gosub_labels))
        if entry in self.hooks:
            head.append("  { extern void %s(u32); %s(r0); }" % (self.hooks[entry], self.hooks[entry]))
        if entry in self.gates:
            head.append("  { extern int %s(u32); if (%s(r0)) %s }" % (self.gates[entry], self.gates[entry], RET))
        body = "\n".join(head) + "\n" + text + "\n  " + RET + "\n}\n"
        if pads:
            body = body.replace(RET, "{ nds_eh_leave(eh_key_); " + RET + " }")
        if name in self.flag_funcs:
            body = body.replace(RET, "{ SAVEF(); " + RET + " }")
        if any(lo <= entry < hi for lo, hi in self.shared_ranges):
            # registres recus et rendus par nds_xr, appels qui les transmettent (CALLX)
            body = body.replace("r4=0,r5=0,r6=0,r7=0,r8=0,r9=0,r10=0,r11=0,r12=0,", "r4=nds_xr[4],r5=nds_xr[5],r6=nds_xr[6],r7=nds_xr[7],r8=nds_xr[8],r9=nds_xr[9],r10=nds_xr[10],r11=nds_xr[11],r12=nds_xr[12],", 1)
            body = body.replace("u32 sp = HOST2DS(", "u32 sp = nds_xr_in ? nds_xr[13] : HOST2DS(", 1)
            body = body.replace("  u32 r4=nds_xr[4]", "  nds_xr_in = 0; u32 r4=nds_xr[4]", 1)
            shared = {n for a, n in self.symbols.items() if any(lo <= a < hi for lo, hi in self.shared_ranges)}
            shared |= {n for a, n in self.new_subs.items() if any(lo <= a < hi for lo, hi in self.shared_ranges)}
            body = re.sub(r"CALLF\(nds_call_target\(([^()]*)\)\)", r"CALLXI(\1)", body)
            body = re.sub(r"CALLF\((\w+)\)", lambda m: ("CALLX(%s)" if m.group(1) in shared else "CALLF(%s)") % m.group(1), body)
            body = body.replace(RET, "{ XR_OUT(); " + RET + " }")
        if entry in self.exit_hooks:
            fn = self.exit_hooks[entry]
            body = body.replace(RET, "{ extern void %s(void); %s(); %s }" % (fn, fn, RET))
        if self.hook_all:
            k = self.hooked.setdefault((entry, name), (name, len(self.hooked)))[1]
            body += "HOOK_ENTRY(%s, %d);\n" % (name, k)
        return body

    def hook_table(self):
        """Fin de lifted.c : les cases des accroches et la table adresse DS -> case, corps d'origine (nitro-pc/hooks.c)."""
        rows = sorted(self.hooked.items())
        out = ["void *nds_hook_slot[%d];" % max(len(rows), 1)]
        out += ["u64 %s__o();" % name for _, (name, _) in rows]
        out.append("const struct { u32 arm; u32 slot; void *orig; } nds_hook_table[] = {")
        out += ["  {0x%08xu, %d, (void *)%s__o}," % (addr, k, name) for (addr, _), (name, k) in rows]
        if not rows:
            out.append("  {0, 0, 0},")
        out.append("};\nconst unsigned nds_hook_count = %d;" % len(rows))
        insns = sorted(self.insn_hooks.items())
        out.append("void *nds_insn_slot[%d];" % max(len(insns), 1))
        out.append("const struct { u32 arm; u32 slot; } nds_insn_table[] = {")
        out += ["  {0x%08xu, %d}," % (addr, k) for addr, k in insns] or ["  {0, 0},"]
        out.append("};\nconst unsigned nds_insn_count = %d;" % len(insns))
        return "\n".join(out) + "\n"

    def landing_pad(self, entry, out):
        """Debut d'un bloc catch pas encore traduit : rien n'y saute, seul le derouleur y arrive. Il finit par __end_catch."""
        if self.eh_end_catch is None:
            return None
        size = self.sizes.get(entry, 0)
        start = None
        for pc in range(entry, entry + size, 4):
            if pc in out:
                start = None
                continue
            if start is None:
                start = pc
            ins = self.decode(pc, False)
            if ins is not None and ins.id == ARM_INS_BL and ins.operands[0].imm & 0xFFFFFFFF == self.eh_end_catch:
                return start
        return None

    def frame_words(self, out):
        """Borne haute de la pile de la fonction : toutes ses descentes de sp additionnees."""
        total = 64
        for text, _, _ in out.values():
            m = re.search(r"sp = sp - (0x[0-9a-f]+)u", text)
            if m:
                total += int(m.group(1), 16)
            m = re.search(r"sp=_a-(\d+);", text)
            if m:
                total += int(m.group(1))
        return min((total + 3) // 4, 1 << 16)
