/* nds_mod.h : interface des mods natifs de Nds Game Studio, version 1.0.
 * SPDX-License-Identifier: MIT
 *
 * Un mod natif est une bibliotheque (.dll, .dylib) qui exporte nds_mod_load. Il ne connait que ce fichier : ni le jeu,
 * ni le runtime, ni le launcher. Le runtime du jeu lui donne la table nds_api au chargement.
 *
 * Regles de la table :
 *  - on n'ajoute qu'a la fin ; une majeure differente est refusee par le runtime ;
 *  - avant d'utiliser un champ ajoute apres la 1.0, verifier NDS_API_HAS(api, champ) ;
 *  - toute adresse est une adresse du DS (0x02000000...) : ne jamais faire *(u32 *)adresse, passer par read32 ou memory.
 *
 * Fils : les accroches, on_frame, on_gx, on_polygon, on_io_*, on_irq et on_rom_read tournent dans le fil du jeu ;
 * on_present tourne dans le fil de dessin, on_audio dans le fil du son. Un mod qui partage des donnees entre eux se
 * protege lui-meme. */
#ifndef NDS_MOD_H
#define NDS_MOD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int nds_u32;
typedef int nds_s32;
typedef unsigned short nds_u16;
typedef short nds_s16;
typedef unsigned char nds_u8;
typedef unsigned long long nds_u64;

#define NDS_API_MAJOR 1
#define NDS_API_MINOR 0

/* Toute fonction traduite du jeu : r0-r3, puis les 10 premiers mots de pile ; retour r0 (32 bits bas) et r1 (hauts). */
typedef nds_u64 (*nds_fn)(nds_u32 r0, nds_u32 r1, nds_u32 r2, nds_u32 r3, nds_u32 a0, nds_u32 a1, nds_u32 a2,
                          nds_u32 a3, nds_u32 a4, nds_u32 a5, nds_u32 a6, nds_u32 a7, nds_u32 a8, nds_u32 a9);

typedef struct nds_mod nds_mod; /* poignee du mod, donnee par le runtime */

/* Registres a une instruction accrochee (hook_insn) : modifiables, ils repartent dans le jeu. pc ne change rien. */
typedef struct {
    nds_u32 r[16];  /* r0..r12, sp (13), lr (14), pc (15) */
    nds_u32 cpsr;   /* N Z C V dans les bits 31..28 */
} nds_regs;

/* Sommet d'un polygone, apres la projection, avant le decoupage (on_polygon). */
typedef struct {
    float x, y, z, w;   /* coordonnees de decoupage : l'ecran va de -w a +w */
    float r, g, b;      /* couleur, 0..31 */
    float s, t;         /* texture, en texels */
} nds_vertex;

typedef struct {
    nds_u32 size;              /* sizeof(nds_input) du runtime */
    nds_u32 ds_keys;           /* bits de KEYINPUT, a 1 = enfonce : A=1 B=2 Select=4 Start=8 droite=16 gauche=32
                                * haut=64 bas=128 R=256 L=512 X=1024 Y=2048 */
    nds_s32 touch_down, touch_x, touch_y; /* stylet, en pixels de l'ecran du bas */
    nds_s32 mouse_dx, mouse_dy;           /* cumul des mouvements de la souris depuis le lancement */
    nds_u32 mouse_buttons;     /* bit 0 gauche, 1 droit, 2 milieu */
    nds_s32 wheel;             /* cumul de la molette */
    nds_u32 keys[8];           /* clavier entier, un bit par touche (codes HID, NDS_KEY_*) */
    nds_u32 window;            /* bit 0 : curseur retenu, bit 1 : fenetre active */
} nds_input;

/* touches du clavier (codes HID d'usage, page 7), par leur place et non leur lettre : NDS_KEY_Q est le A d'un clavier AZERTY */
#define NDS_KEY_DOWN(in, k) ((in)->keys[(k) >> 5] >> ((k) & 31) & 1)
enum {
    NDS_KEY_A = 0x04, NDS_KEY_B, NDS_KEY_C, NDS_KEY_D, NDS_KEY_E, NDS_KEY_F, NDS_KEY_G, NDS_KEY_H, NDS_KEY_I, NDS_KEY_J,
    NDS_KEY_K, NDS_KEY_L, NDS_KEY_M, NDS_KEY_N, NDS_KEY_O, NDS_KEY_P, NDS_KEY_Q, NDS_KEY_R, NDS_KEY_S, NDS_KEY_T, NDS_KEY_U,
    NDS_KEY_V, NDS_KEY_W, NDS_KEY_X, NDS_KEY_Y, NDS_KEY_Z,
    NDS_KEY_1 = 0x1E, NDS_KEY_2, NDS_KEY_3, NDS_KEY_4, NDS_KEY_5, NDS_KEY_6, NDS_KEY_7, NDS_KEY_8, NDS_KEY_9, NDS_KEY_0,
    NDS_KEY_ENTER = 0x28, NDS_KEY_ESCAPE, NDS_KEY_BACKSPACE, NDS_KEY_TAB, NDS_KEY_SPACE,
    NDS_KEY_F1 = 0x3A, NDS_KEY_F2, NDS_KEY_F3, NDS_KEY_F4, NDS_KEY_F5, NDS_KEY_F6, NDS_KEY_F7, NDS_KEY_F8, NDS_KEY_F9,
    NDS_KEY_F10, NDS_KEY_F11, NDS_KEY_F12,
    NDS_KEY_INSERT = 0x49, NDS_KEY_HOME, NDS_KEY_PAGE_UP, NDS_KEY_DELETE, NDS_KEY_END, NDS_KEY_PAGE_DOWN,
    NDS_KEY_RIGHT = 0x4F, NDS_KEY_LEFT, NDS_KEY_DOWN, NDS_KEY_UP,
    NDS_KEY_LEFT_CTRL = 0xE0, NDS_KEY_LEFT_SHIFT, NDS_KEY_LEFT_ALT, NDS_KEY_LEFT_META,
    NDS_KEY_RIGHT_CTRL, NDS_KEY_RIGHT_SHIFT, NDS_KEY_RIGHT_ALT, NDS_KEY_RIGHT_META,
};

typedef struct nds_api {
    nds_u32 size;              /* sizeof(nds_api) du runtime : un champ au-dela n'existe pas */
    nds_u32 major, minor;      /* version de l'interface du runtime */
    const char *game_code;     /* "AL7E" */
    const char *platform;      /* "windows-x86", "windows-x86_64", "macos-arm64", "macos-x86_64" */
    const char *language;      /* langue du launcher : "fr", "en" */

    /* ---- journal, reglages */
    void (*log)(nds_mod *mod, const char *fmt, ...);      /* une ligne ; %x %u %d %s ; prefixee du nom du mod */
    void (*fail)(nds_mod *mod, const char *reason);       /* le mod se retire : ses accroches et rappels sont coupes */
    const char *(*setting)(nds_mod *mod, const char *key); /* valeur du reglage (texte), 0 si absent */

    /* ---- memoire du DS (tout l'espace : RAM, VRAM, registres) */
    nds_u8 *(*memory)(nds_u32 ds_addr); /* pointeur de l'hote ; pas pour les registres (0x04000000), qui ont des effets */
    nds_u32 (*read8)(nds_u32 ds_addr);
    nds_u32 (*read16)(nds_u32 ds_addr);
    nds_u32 (*read32)(nds_u32 ds_addr);
    void (*write8)(nds_u32 ds_addr, nds_u32 value);
    void (*write16)(nds_u32 ds_addr, nds_u32 value);
    void (*write32)(nds_u32 ds_addr, nds_u32 value);

    /* ---- code du jeu */
    /* Remplace la fonction qui commence a ds_addr ; *original recoit la suite (mod precedent, ou le jeu), a appeler ou
     * non. Le dernier mod charge passe en premier. 0 si accroche, -1 si ds_addr n'est pas une fonction du jeu. */
    int (*hook)(nds_mod *mod, nds_u32 ds_addr, void *replacement, void **original);
    /* Appelle fn avant l'instruction a ds_addr. Il faut que le module ait ete genere avec cette adresse : le manifeste
     * la declare dans "instruction_hooks". -1 sinon. */
    int (*hook_insn)(nds_mod *mod, nds_u32 ds_addr, void (*fn)(void *user, nds_regs *regs), void *user);
    nds_fn (*function_at)(nds_u32 ds_addr); /* pour appeler une fonction du jeu (bit Thumb accepte) ; 0 si aucune */

    /* ---- rappels : autant qu'on veut, dans l'ordre d'inscription */
    void (*on_frame)(nds_mod *mod, void (*fn)(void *user, nds_u32 frame), void *user); /* a chaque VBlank */
    /* commande 3D (registres 0x04000400-0x040005FF, FIFO) : renvoyer 1 l'enleve */
    void (*on_gx)(nds_mod *mod, int (*fn)(void *user, nds_u32 cmd, nds_u32 *params, nds_u32 count), void *user);
    /* polygone (3 ou 4 sommets, modifiables) : renvoyer 1 l'enleve. attr, tex, pal : POLYGON_ATTR, TEXIMAGE_PARAM, PLTT_BASE */
    void (*on_polygon)(nds_mod *mod, int (*fn)(void *user, nds_vertex *v, int n, nds_u32 attr, nds_u32 tex, nds_u32 pal), void *user);
    /* images finales des deux ecrans, w x h pixels, stride en pixels ; le mod peut y dessiner. Un pixel = octets R, G, B, A en
     * memoire, soit 0xAABBGGRR en nds_u32 (NDS_RGB) */
    void (*on_present)(nds_mod *mod, void (*fn)(void *user, nds_u32 *top, nds_u32 *bottom, int w, int h, int stride), void *user);
    /* son mixe, stereo 16 bits, 32768 Hz, modifiable */
    void (*on_audio)(nds_mod *mod, void (*fn)(void *user, nds_s16 *stereo, nds_u32 frames), void *user);
    /* lecture d'un registre par le jeu : *value est ce qu'il lira */
    void (*on_io_read)(nds_mod *mod, void (*fn)(void *user, nds_u32 addr, nds_u32 size, nds_u32 *value), void *user);
    /* ecriture d'un registre par le jeu : *value modifiable, renvoyer 1 l'empeche */
    void (*on_io_write)(nds_mod *mod, int (*fn)(void *user, nds_u32 addr, nds_u32 size, nds_u32 *value), void *user);
    /* interruptions sur le point d'etre servies (bits de IF) */
    void (*on_irq)(nds_mod *mod, void (*fn)(void *user, nds_u32 bits), void *user);
    /* lecture de la rom par le jeu : dst (deja rempli) peut etre modifie ; pos = position dans la rom */
    void (*on_rom_read)(nds_mod *mod, void (*fn)(void *user, nds_u32 pos, nds_u32 size, nds_u8 *dst), void *user);

    /* ---- entrees : touches du DS, stylet, souris et clavier entier, tels que le launcher les envoie */
    void (*input)(nds_input *out);              /* out->size doit etre rempli par le mod */
    void (*take_input)(nds_mod *mod, int on);   /* 1 : le jeu ne recoit plus de touches */
    void (*capture_mouse)(nds_mod *mod, int on); /* 1 : le launcher retient le curseur (F7 le rend au joueur) */
    void (*notice)(nds_mod *mod, const char *text, nds_u32 ms); /* message affiche par le launcher (UTF-8) */

    /* ---- tout le reste : fonction ou variable du runtime par son nom, 0 si absente. Sans promesse de stabilite :
     * le nom peut disparaitre a une version suivante du runtime. */
    void *(*runtime)(const char *name);

    /* ---- le corps traduit d'une fonction du jeu, sans aucune accroche de mod (ni la sienne, ni celle des autres) : pour
     * qu'un mod passe avant tous les autres, ou appelle le jeu tel quel. 0 si ds_addr n'est pas une fonction traduite. */
    nds_fn (*game_function)(nds_u32 ds_addr);
} nds_api;

/* un pixel de on_present : octets R, G, B, A */
#define NDS_RGB(r, g, b) (0xFF000000u | ((nds_u32)(b) << 16) | ((nds_u32)(g) << 8) | (nds_u32)(r))

/* le champ existe dans la table de ce runtime */
#define NDS_API_HAS(api, field) ((api)->size >= (nds_u32)((char *)&((nds_api *)0)->field - (char *)0) + sizeof((api)->field))

#ifdef _WIN32
#define NDS_MOD_EXPORT __declspec(dllexport)
#else
#define NDS_MOD_EXPORT __attribute__((visibility("default")))
#endif

/* Appelee une fois, avant la premiere instruction du jeu. 0 = charge ; sinon le mod est refuse (api->fail donne la raison). */
NDS_MOD_EXPORT int nds_mod_load(const nds_api *api, nds_mod *mod);

#ifdef __cplusplus
}
#endif
#endif
