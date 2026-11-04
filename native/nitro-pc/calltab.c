/* Pointeurs de fonctions du jeu : adresse DS (bit Thumb compris) -> fonction de l'hote (native/MACOS.md 3.2).
 * Les pointeurs de code restent des adresses ARM partout (memoire DS, registres du code traduit, rappels du SDK) :
 * chaque appel indirect passe par nds_call_target. Sans en-tete systeme ni assembleur : le test sans rom
 * (native/tests) compile ce fichier tel quel. */
#include "nitro.h"

extern const struct { u32 arm; void *x86; } nds_fn_table[]; /* trie par adresse, bit Thumb enleve (tables.c) */
extern const unsigned nds_fn_count;

/* Table directe sur l'ITCM (0x01FF8000) et la RAM principale (jusqu'a 0x02400000, overlays compris), une entree
 * tous les 2 octets : un seul chargement par appel. 8 Mo (32 bits) ou 16 Mo (64 bits) de .bss, que le systeme
 * n'engage qu'aux pages touchees. */
#define CODE_LO 0x01FF8000u
#define CODE_HI 0x02400000u
#define CODE_N ((CODE_HI - CODE_LO) / 2)
static void *code_map[CODE_N];

void nds_code_map_init(void) {
    for (unsigned i = 0; i < nds_fn_count; i++) {
        u32 k = ((nds_fn_table[i].arm & ~1u) - CODE_LO) >> 1;
        if (k < CODE_N && !code_map[k]) code_map[k] = nds_fn_table[i].x86;
    }
}

/* fonctions hors de la table directe (DTCM, miroirs) : recherche dans nds_fn_table */
static uptr call_target_slow(u32 a) {
    u32 key = a & ~1u;
    unsigned lo = 0, hi = nds_fn_count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (nds_fn_table[mid].arm < key) lo = mid + 1;
        else hi = mid;
    }
    return lo < nds_fn_count && nds_fn_table[lo].arm == key ? (uptr)nds_fn_table[lo].x86 : 0;
}

uptr nds_call_target_or_0(u32 a) {
    u32 k = (a - CODE_LO) >> 1;
    return k < CODE_N ? (uptr)code_map[k] : call_target_slow(a);
}

uptr nds_call_target(u32 a) {
    u32 k = (a - CODE_LO) >> 1; /* le bit Thumb tombe */
    uptr f = k < CODE_N ? (uptr)code_map[k] : call_target_slow(a); /* dans la table directe, rien d'autre a chercher */
    if (!f) nds_fatal("saut vers une adresse DS sans fonction : %x", a);
    return f;
}

/* ------------------------------------------------------------------ accroches des mods (mods.c)
 * fin de lifted.c (arm2c.Lifter.hook_table) : une case par fonction traduite, triee par adresse DS */
extern void *nds_hook_slot[];
extern const struct { u32 arm; u32 slot; void *orig; } nds_hook_table[];
extern const unsigned nds_hook_count;

/* Le dernier installe passe en premier : *original recoit l'accroche precedente, ou le corps traduit. */
int nds_hook_install(u32 addr, void *fn, void **original) {
    u32 key = addr & ~1u;
    unsigned lo = 0, hi = nds_hook_count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (nds_hook_table[mid].arm < key) lo = mid + 1;
        else hi = mid;
    }
    if (lo >= nds_hook_count || nds_hook_table[lo].arm != key) return -1;
    void **slot = &nds_hook_slot[nds_hook_table[lo].slot];
    if (original) *original = *slot ? *slot : nds_hook_table[lo].orig;
    *slot = fn;
    return 0;
}

static int hook_find(u32 addr) {
    u32 key = addr & ~1u;
    unsigned lo = 0, hi = nds_hook_count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (nds_hook_table[mid].arm < key) lo = mid + 1;
        else hi = mid;
    }
    return lo < nds_hook_count && nds_hook_table[lo].arm == key ? (int)lo : -1;
}

/* le corps traduit, sans accroche (api->game_function des mods) ; 0 si addr n'est pas une fonction traduite */
void *nds_hook_body(u32 addr) {
    int i = hook_find(addr);
    return i < 0 ? 0 : nds_hook_table[i].orig;
}

/* ce qu'un appel a addr execute maintenant : la derniere accroche, ou le corps traduit ; -1 si pas une fonction */
int nds_hook_peek(u32 addr, void **current) {
    int i = hook_find(addr);
    if (i < 0) return -1;
    void *slot = nds_hook_slot[nds_hook_table[i].slot];
    *current = slot ? slot : nds_hook_table[i].orig;
    return 0;
}
