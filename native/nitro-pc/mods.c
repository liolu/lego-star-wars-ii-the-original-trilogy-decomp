/* Mods natifs (ROADMAP-MODS.md, etape 1) : la table nds_api de native/sdk/nds_mod.h, le chargement et les rappels.
 *
 *   --plugins fichier.json  liste ecrite par le launcher (ordre, bibliotheque par plateforme, jeux, interface, reglages) ;
 *                           l'etat de chaque mod va dans status.json, a cote (ou --mod-status fichier)
 *   --plugin a.dll,b.dll    essais : bibliotheques sans manifeste
 *   --no-mods               aucun mod (comparaisons image par image)
 *
 * Les cases des accroches de fonctions et nds_hook_install sont dans calltab.c ; ici, ce que les mods inscrivent.
 * Sans en-tete systeme : la boite a outils du launcher n'en livre aucun. */
#include "nitro.h"
#include "../sdk/nds_mod.h"

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
void nds_vlog(const char *fmt, va_list ap);
u32 nds_io_read(u32 a, u32 size);
void nds_io_write(u32 a, u32 v, u32 size);

#if defined(_WIN32) && defined(__i386__)
#define PLATFORM "windows-x86"
#elif defined(_WIN32) && defined(__x86_64__)
#define PLATFORM "windows-x86_64"
#elif defined(__APPLE__) && defined(__aarch64__)
#define PLATFORM "macos-arm64"
#elif defined(__APPLE__) && defined(__x86_64__)
#define PLATFORM "macos-x86_64"
#else
#define PLATFORM "inconnue"
#endif

/* ------------------------------------------------------------------ memoire : par blocs, sans plafond */
static u8 *arena;
static u32 arena_left;

static void *mods_alloc(u32 size) {
    size = (size + 15) & ~15u;
    if (size > arena_left) {
        u32 block = size > 0x10000 ? size : 0x10000;
        arena = os_mem_alloc(0, block);
        if (!arena) nds_fatal("mods : memoire epuisee");
        arena_left = block;
    }
    void *p = arena;
    arena += size;
    arena_left -= size;
    return p;
}

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) a++, b++;
    return *a == *b;
}

static u32 str_len(const char *s) {
    u32 n = 0;
    while (s[n]) n++;
    return n;
}

static char *str_dup(const char *s, u32 n) {
    char *d = mods_alloc(n + 1);
    nds_copy(d, s, n);
    d[n] = 0;
    return d;
}

/* ------------------------------------------------------------------ JSON (plugins.json), lu sur place */
typedef struct Json Json;
struct Json {
    char type;           /* o objet, a liste, s texte, n nombre ou true/false/null */
    const char *key;     /* dans un objet */
    const char *text;    /* s, n */
    Json *child, *next;
};

static char *jp;

static void j_space(void) { while (*jp == ' ' || *jp == '\t' || *jp == '\n' || *jp == '\r') jp++; }

static char *j_string(void) {
    char *out = ++jp, *w = out;
    while (*jp && *jp != '"') {
        if (*jp == '\\' && jp[1]) {
            jp++;
            char c = *jp == 'n' ? '\n' : *jp == 't' ? '\t' : *jp;
            if (*jp == 'u') { c = '?'; jp += jp[1] && jp[2] && jp[3] && jp[4] ? 4 : 0; } /* hors ASCII : remplace */
            *w++ = c;
            jp++;
        } else *w++ = *jp++;
    }
    if (*jp) jp++;
    *w = 0; /* le texte decode est plus court : il finit au plus sur le guillemet */
    return out;
}

static Json *j_value(void) {
    j_space();
    Json *v = mods_alloc(sizeof(Json));
    nds_zero(v, sizeof(Json));
    if (*jp == '{' || *jp == '[') {
        char close = *jp == '{' ? '}' : ']';
        v->type = *jp == '{' ? 'o' : 'a';
        jp++;
        Json **tail = &v->child;
        for (;;) {
            j_space();
            if (*jp == close) { jp++; break; }
            if (!*jp) return 0;
            const char *key = 0;
            if (v->type == 'o') {
                if (*jp != '"') return 0;
                key = j_string();
                j_space();
                if (*jp++ != ':') return 0;
            }
            Json *c = j_value();
            if (!c) return 0;
            c->key = key;
            *tail = c;
            tail = &c->next;
            j_space();
            if (*jp == ',') jp++;
        }
    } else if (*jp == '"') {
        v->type = 's';
        v->text = j_string();
    } else {
        const char *s = jp;
        while (*jp && *jp != ',' && *jp != '}' && *jp != ']' && *jp != ' ' && *jp != '\n' && *jp != '\r' && *jp != '\t') jp++;
        if (jp == s) return 0;
        v->type = 'n';
        v->text = str_dup(s, (u32)(jp - s));
    }
    return v;
}

static Json *j_get(const Json *o, const char *key) {
    for (Json *c = o ? o->child : 0; c; c = c->next)
        if (c->key && str_eq(c->key, key)) return c;
    return 0;
}

static const char *j_text(const Json *o, const char *key) {
    Json *v = j_get(o, key);
    return v && (v->type == 's' || v->type == 'n') ? v->text : 0;
}

/* ------------------------------------------------------------------ mods */
struct nds_mod {
    const char *id, *version, *path;
    Json *settings;
    const char *state, *reason; /* "charge", "refuse", "retire" */
    int net_safe; /* "multiplayer": "safe" : sans effet sur la partie, pas compare en multi */
    int active, loading;
    struct nds_mod *next;
};

typedef struct HookRec { nds_mod *mod; u32 addr; void *fn, *before; struct HookRec *next; } HookRec;
static nds_mod *mods, **mods_tail = &mods;
static HookRec *hook_recs;

/* ------------------------------------------------------------------ rappels */
typedef struct Cb { void *fn, *user; nds_mod *mod; struct Cb *next; } Cb;
NdsCallbacks nds_cb;

static void cb_add(void **head, nds_mod *mod, void *fn, void *user) {
    if (!fn) return;
    Cb *c = mods_alloc(sizeof(Cb));
    c->fn = fn;
    c->user = user;
    c->mod = mod;
    c->next = 0;
    Cb **at = (Cb **)head;
    while (*at) at = &(*at)->next;
    *at = c; /* pose en dernier : les fils de dessin et de son parcourent la liste sans verrou */
}

#define EACH(list) for (Cb *c = (Cb *)nds_cb.list; c; c = c->next) if (c->mod->active)

void nds_mods_frame(u32 frame) {
    EACH(frame) ((void (*)(void *, u32))c->fn)(c->user, frame);
}

int nds_mods_gx(u32 cmd, u32 *params, u32 count) {
    EACH(gx) if (((int (*)(void *, u32, u32 *, u32))c->fn)(c->user, cmd, params, count)) return 1;
    return 0;
}

int nds_mods_polygon(void *v, int n, u32 attr, u32 tex, u32 pal) {
    EACH(polygon) if (((int (*)(void *, void *, int, u32, u32, u32))c->fn)(c->user, v, n, attr, tex, pal)) return 1;
    return 0;
}

void nds_mods_present(u32 *top, u32 *bottom, int w, int h, int stride) {
    EACH(present) ((void (*)(void *, u32 *, u32 *, int, int, int))c->fn)(c->user, top, bottom, w, h, stride);
}

void nds_mods_audio(s16 *stereo, u32 frames) {
    EACH(audio) ((void (*)(void *, s16 *, u32))c->fn)(c->user, stereo, frames);
}

u32 nds_mods_io_read(u32 addr, u32 size, u32 value) {
    EACH(io_read) ((void (*)(void *, u32, u32, u32 *))c->fn)(c->user, addr, size, &value);
    return value;
}

int nds_mods_io_write(u32 addr, u32 size, u32 *value) {
    EACH(io_write) if (((int (*)(void *, u32, u32, u32 *))c->fn)(c->user, addr, size, value)) return 1;
    return 0;
}

void nds_mods_irq(u32 bits) {
    EACH(irq) ((void (*)(void *, u32))c->fn)(c->user, bits);
}

void nds_mods_rom_read(u32 pos, u32 size, u8 *dst) {
    EACH(rom_read) ((void (*)(void *, u32, u32, u8 *))c->fn)(c->user, pos, size, dst);
}

/* accroches a une instruction : fin de lifted.c (arm2c.Lifter.insn_table), une liste de rappels par case */
extern void *nds_insn_slot[];
extern const struct { u32 arm; u32 slot; } nds_insn_table[];
extern const unsigned nds_insn_count;

void nds_insn_run(u32 k, u32 *regs) {
    for (Cb *c = nds_insn_slot[k]; c; c = c->next)
        if (c->mod->active) ((void (*)(void *, nds_regs *))c->fn)(c->user, (nds_regs *)regs);
}

/* ------------------------------------------------------------------ la table */
static void api_log(nds_mod *mod, const char *fmt, ...) {
    char line[1024];
    u32 n = 0;
    const char *id = mod ? mod->id : "?";
    line[n++] = '[';
    while (*id && n < 100) {
        if (*id == '%') line[n++] = '%';
        line[n++] = *id++;
    }
    line[n++] = ']';
    line[n++] = ' ';
    for (const char *f = fmt; *f && n < sizeof line - 1; f++) line[n++] = *f;
    line[n] = 0;
    va_list ap;
    va_start(ap, fmt);
    nds_vlog(line, ap);
    va_end(ap);
}

static void write_status(void);

/* une accroche se retire si personne ne s'est mis devant ; sinon elle reste, le mod inactif ne compte plus */
static void unhook_mod(nds_mod *mod) {
    for (HookRec *h = hook_recs; h; h = h->next) {
        if (h->mod != mod || !h->fn) continue;
        void *top;
        if (nds_hook_peek(h->addr, &top) == 0 && top == h->fn) nds_hook_install(h->addr, h->before, 0);
        else nds_log("[%s] accroche %x laissee en place : un autre mod s'est mis devant", mod->id, h->addr);
        h->fn = 0;
    }
}

static void api_fail(nds_mod *mod, const char *reason) {
    if (!mod || !mod->active) return;
    if (!reason) reason = "sans raison";
    mod->active = 0;
    mod->state = mod->loading ? "refuse" : "retire";
    mod->reason = str_dup(reason, str_len(reason));
    nds_log("[%s] %s : %s", mod->id, mod->state, mod->reason);
    unhook_mod(mod);
    if (!mod->loading) write_status();
}

static const char *api_setting(nds_mod *mod, const char *key) { return mod ? j_text(mod->settings, key) : 0; }

#define API_IO(a) (((a) >> 24) == 4 || (a) < 0x01000000u || (s32)(a) < 0)
static u8 *api_memory(u32 a) { return DS(a); }
static u32 api_read8(u32 a) { return API_IO(a) ? nds_io_read(a, 1) : REG8(a); }
static u32 api_read16(u32 a) { return API_IO(a) ? nds_io_read(a & ~1u, 2) : REG16(a & ~1u); }
static u32 api_read32(u32 a) { return API_IO(a) ? nds_io_read(a & ~3u, 4) : REG32(a & ~3u); }
static void api_write8(u32 a, u32 v) { if (API_IO(a)) nds_io_write(a, v, 1); else REG8(a) = (u8)v; }
static void api_write16(u32 a, u32 v) { if (API_IO(a)) nds_io_write(a & ~1u, v, 2); else REG16(a & ~1u) = (u16)v; }
static void api_write32(u32 a, u32 v) { if (API_IO(a)) nds_io_write(a & ~3u, v, 4); else REG32(a & ~3u) = v; }

static int api_hook(nds_mod *mod, u32 addr, void *fn, void **original) {
    void *before;
    if (nds_hook_peek(addr, &before)) {
        nds_log("[%s] accroche refusee : %x n'est pas l'entree d'une fonction traduite du jeu", mod ? mod->id : "?", addr);
        return -1;
    }
    nds_hook_install(addr, fn, original);
    HookRec *h = mods_alloc(sizeof(HookRec));
    h->mod = mod;
    h->addr = addr & ~1u;
    h->fn = fn;
    h->before = before;
    h->next = hook_recs;
    hook_recs = h;
    return 0;
}

static int api_hook_insn(nds_mod *mod, u32 addr, void (*fn)(void *, nds_regs *), void *user) {
    for (unsigned i = 0; i < nds_insn_count; i++) {
        if (nds_insn_table[i].arm != (addr & ~1u)) continue;
        cb_add(&nds_insn_slot[nds_insn_table[i].slot], mod, (void *)fn, user);
        return 0;
    }
    nds_log("[%s] accroche a l'instruction %x refusee : le module n'a pas ete genere avec elle (\"instruction_hooks\" du manifeste)",
            mod ? mod->id : "?", addr);
    return -1;
}

static nds_fn api_function_at(u32 addr) { return (nds_fn)nds_call_target_or_0(addr); }
static nds_fn api_game_function(u32 addr) { return (nds_fn)nds_hook_body(addr); }

static void api_on_frame(nds_mod *m, void (*fn)(void *, u32), void *u) { cb_add(&nds_cb.frame, m, (void *)fn, u); }
static void api_on_gx(nds_mod *m, int (*fn)(void *, u32, u32 *, u32), void *u) { cb_add(&nds_cb.gx, m, (void *)fn, u); }
static void api_on_polygon(nds_mod *m, int (*fn)(void *, nds_vertex *, int, u32, u32, u32), void *u) { cb_add(&nds_cb.polygon, m, (void *)fn, u); }
static void api_on_present(nds_mod *m, void (*fn)(void *, u32 *, u32 *, int, int, int), void *u) { cb_add(&nds_cb.present, m, (void *)fn, u); }
static void api_on_audio(nds_mod *m, void (*fn)(void *, s16 *, u32), void *u) { cb_add(&nds_cb.audio, m, (void *)fn, u); }
static void api_on_io_read(nds_mod *m, void (*fn)(void *, u32, u32, u32 *), void *u) { cb_add(&nds_cb.io_read, m, (void *)fn, u); }
static void api_on_io_write(nds_mod *m, int (*fn)(void *, u32, u32, u32 *), void *u) { cb_add(&nds_cb.io_write, m, (void *)fn, u); }
static void api_on_irq(nds_mod *m, void (*fn)(void *, u32), void *u) { cb_add(&nds_cb.irq, m, (void *)fn, u); }
static void api_on_rom_read(nds_mod *m, void (*fn)(void *, u32, u32, u8 *), void *u) { cb_add(&nds_cb.rom_read, m, (void *)fn, u); }

int nds_mods_input_taken, nds_mods_mouse_captured;

static void api_input(nds_input *out) {
    u32 size = out->size < sizeof(nds_input) ? out->size : sizeof(nds_input);
    nds_input in;
    nds_zero(&in, sizeof in);
    in.size = size;
    in.ds_keys = present_pad() & 0xFFF;
    present_touch(&in.touch_down, &in.touch_x, &in.touch_y);
    present_mouse(&in.mouse_dx, &in.mouse_dy);
    present_input_extra(&in.mouse_buttons, &in.wheel, in.keys);
    in.window = present_window_flags();
    nds_copy(out, &in, size);
}

static void api_take_input(nds_mod *mod, int on) { nds_mods_input_taken = on != 0; }
static void api_capture_mouse(nds_mod *mod, int on) {
    nds_mods_mouse_captured = on != 0;
    present_flags_changed();
}
static void api_notice(nds_mod *mod, const char *text, u32 ms) {
    nds_log("[%s] message : %s", mod ? mod->id : "?", text);
    present_notice(text, ms);
}

/* variables et fonctions du runtime par leur nom (tables.c de native_build.py) */
extern const struct { const char *name; void *p; } nds_runtime_syms[];
extern const unsigned nds_runtime_sym_count;

static void *api_runtime(const char *name) {
    for (unsigned i = 0; i < nds_runtime_sym_count; i++)
        if (str_eq(nds_runtime_syms[i].name, name)) return nds_runtime_syms[i].p;
    return 0;
}

static char game_code[5];
static nds_api api = { /* language : au chargement, d'apres --lang */
    sizeof(nds_api), NDS_API_MAJOR, NDS_API_MINOR, game_code, PLATFORM, 0,
    api_log, api_fail, api_setting,
    api_memory, api_read8, api_read16, api_read32, api_write8, api_write16, api_write32,
    api_hook, api_hook_insn, api_function_at,
    api_on_frame, api_on_gx, api_on_polygon, api_on_present, api_on_audio, api_on_io_read, api_on_io_write, api_on_irq,
    api_on_rom_read,
    api_input, api_take_input, api_capture_mouse, api_notice,
    api_runtime,
    api_game_function,
};

/* ------------------------------------------------------------------ status.json */
static const char *status_path;

static u32 put(char *buf, u32 n, u32 max, const char *s, int quote) {
    if (quote && n < max) buf[n++] = '"';
    for (; *s && n + 3 < max; s++) {
        if (quote && (*s == '"' || *s == '\\')) buf[n++] = '\\';
        buf[n++] = quote && *s == '\n' ? ' ' : *s;
    }
    if (quote && n < max) buf[n++] = '"';
    return n;
}

static void write_status(void) {
    if (!status_path) return;
    u32 max = 0x1000, n = 0;
    for (nds_mod *m = mods; m; m = m->next) max += 256 + str_len(m->id) * 2 + (m->reason ? str_len(m->reason) * 2 : 0);
    char *buf = mods_alloc(max);
    n = put(buf, n, max, "{\"api\": \"1.0\", \"platform\": \"" PLATFORM "\", \"mods\": [", 0);
    for (nds_mod *m = mods; m; m = m->next) {
        n = put(buf, n, max, m == mods ? "\n  {\"id\": " : ",\n  {\"id\": ", 0);
        n = put(buf, n, max, m->id, 1);
        n = put(buf, n, max, ", \"state\": ", 0);
        n = put(buf, n, max, m->state, 1);
        if (m->reason) {
            n = put(buf, n, max, ", \"reason\": ", 0);
            n = put(buf, n, max, m->reason, 1);
        }
        n = put(buf, n, max, "}", 0);
    }
    n = put(buf, n, max, "\n]}\n", 0);
    if (os_file_save(status_path, buf, n)) nds_log("mods : %s impossible a ecrire", status_path);
}

/* journal d'un plantage : quels mods etaient charges */
void nds_mods_crash_note(void) {
    char line[900];
    u32 n = 0;
    for (nds_mod *m = mods; m; m = m->next) {
        if (!m->active) continue;
        if (n) n = put(line, n, sizeof line, ", ", 0);
        n = put(line, n, sizeof line, m->id, 0);
    }
    line[n] = 0;
    if (n) nds_log("mods charges : %s (relancer sans mods pour savoir si l'un d'eux est en cause)", line);
}

/* Ce que deux joueurs doivent avoir en commun (net.c) : la version du runtime et les mods charges qui touchent a la partie. */
extern const char nds_runtime_version[];

const char *nds_mods_fingerprint(void) {
    static char out[2048];
    u32 n = put(out, 0, sizeof out, "runtime ", 0);
    n = put(out, n, sizeof out, nds_runtime_version, 0);
    for (nds_mod *m = mods; m; m = m->next) {
        if (!m->active || m->net_safe) continue;
        n = put(out, n, sizeof out, ", ", 0);
        n = put(out, n, sizeof out, m->id, 0);
        n = put(out, n, sizeof out, " ", 0);
        n = put(out, n, sizeof out, m->version ? m->version : "?", 0);
    }
    out[n] = 0;
    return out;
}

/* ------------------------------------------------------------------ chargement */
static nds_mod *new_mod(const char *id, const char *version, const char *path) {
    nds_mod *m = mods_alloc(sizeof(nds_mod));
    nds_zero(m, sizeof *m);
    m->id = id ? id : path ? path : "?";
    m->version = version;
    m->path = path;
    m->state = "refuse";
    *mods_tail = m;
    mods_tail = &m->next;
    return m;
}

static void refuse(nds_mod *m, const char *reason, const char *detail) {
    u32 a = str_len(reason), b = detail ? str_len(detail) : 0;
    char *r = mods_alloc(a + b + 1);
    nds_copy(r, reason, a);
    if (b) nds_copy(r + a, detail, b);
    r[a + b] = 0;
    m->active = 0;
    m->state = "refuse";
    m->reason = r;
    nds_log("mod refuse [%s] : %s", m->id, r);
}

static void load_one(nds_mod *m) {
    void *lib = os_lib_open(m->path);
    if (!lib) { refuse(m, "la bibliotheque ne se charge pas : ", m->path); return; }
    int (*load)(const nds_api *, nds_mod *) = (int (*)(const nds_api *, nds_mod *))os_lib_sym(lib, "nds_mod_load");
    if (!load) { refuse(m, "pas de fonction nds_mod_load dans ", m->path); return; }
    m->active = 1;
    m->loading = 1;
    int r = load(&api, m);
    m->loading = 0;
    if (!m->active || r) {
        if (m->active) { m->active = 0; unhook_mod(m); }
        if (!m->reason) refuse(m, "nds_mod_load a renvoye une erreur", 0);
        m->state = "refuse";
        nds_log("mod refuse [%s] : %s", m->id, m->reason);
        return;
    }
    m->state = "charge";
    nds_log("mod charge [%s] %s", m->id, m->version ? m->version : "");
}

/* "1.2" -> majeure 1, mineure 2 */
static int parse_version(const char *s, u32 *major, u32 *minor) {
    *major = *minor = 0;
    if (!s || *s < '0' || *s > '9') return -1;
    while (*s >= '0' && *s <= '9') *major = *major * 10 + *s++ - '0';
    if (*s == '.') for (s++; *s >= '0' && *s <= '9'; s++) *minor = *minor * 10 + *s - '0';
    return 0;
}

/* "games" : liste de codes, ou objet code -> SHA-1 des roms (verifies par le launcher) ; absent = tous les jeux */
static int game_listed(const Json *games) {
    if (!games || !games->child) return 1; /* liste vide aussi : tous les jeux */
    for (Json *g = games->child; g; g = g->next) {
        const char *code = games->type == 'o' ? g->key : g->text;
        if (code && str_eq(code, game_code)) return 1;
    }
    return 0;
}

static void load_manifest_list(const char *path) {
    u32 size;
    char *text = os_file_load(path, &size);
    if (!text) { nds_log("mods : %s introuvable, le jeu demarre sans mods", path); return; }
    char *copy = mods_alloc(size + 1);
    nds_copy(copy, text, size);
    copy[size] = 0;
    jp = copy;
    Json *root = j_value();
    Json *list = root ? j_get(root, "mods") : 0;
    if (!list || list->type != 'a') { nds_log("mods : %s illisible, le jeu demarre sans mods", path); return; }
    for (Json *e = list->child; e; e = e->next) {
        const char *id = j_text(e, "id");
        Json *plugin = j_get(e, "plugin");
        const char *lib = plugin && plugin->type == 'o' ? j_text(plugin, PLATFORM) : plugin ? plugin->text : 0;
        nds_mod *m = new_mod(id, j_text(e, "version"), lib);
        m->settings = j_get(e, "settings");
        { const char *mp = j_text(e, "multiplayer"); m->net_safe = mp && str_eq(mp, "safe"); }
        u32 major, minor;
        const char *want = j_text(e, "api");
        const char *pre = j_text(e, "refused"); /* ce que le launcher est seul a savoir (la rom) */
        if (!id) refuse(m, "manifeste sans \"id\"", 0);
        else if (pre) refuse(m, pre, 0);
        else if (parse_version(want, &major, &minor)) refuse(m, "manifeste sans \"api\"", 0);
        else if (major != NDS_API_MAJOR) refuse(m, "interface d'une autre version majeure que 1 : ", want);
        else if (minor > NDS_API_MINOR) refuse(m, "interface plus recente que celle de ce runtime (1.0), mettre le jeu a jour : ", want);
        else if (!game_listed(j_get(e, "games"))) refuse(m, "pas fait pour ce jeu : ", game_code);
        else if (!lib) refuse(m, "pas de bibliotheque pour cette machine : ", PLATFORM);
        else load_one(m);
    }
}

void nds_mods_load(void) {
    if (nds_arg("--no-mods", 0)) return;
    nds_copy(game_code, DS(0x027FFE0Cu), 4); /* en-tete de la carte, copie au chargement de la rom */
    api.language = present_language();
    const char *list;
    if (nds_arg("--plugins", &list) && list) {
        static char derived[600];
        const char *out = 0;
        if (!nds_arg("--mod-status", &out) || !out) {
            u32 n = str_len(list), cut = 0;
            for (u32 i = 0; i < n; i++)
                if (list[i] == '/' || list[i] == '\\') cut = i + 1;
            if (cut + 12 < sizeof derived) {
                nds_copy(derived, list, cut);
                nds_copy(derived + cut, "status.json", 12);
                out = derived;
            }
        }
        status_path = out;
        load_manifest_list(list);
    }
    if (nds_arg("--plugin", &list) && list) {
        char path[512];
        while (*list) {
            u32 n = 0;
            while (*list && *list != ',' && n < sizeof path - 1) path[n++] = *list++;
            if (*list == ',') list++;
            load_one(new_mod(0, 0, str_dup(path, n)));
        }
    }
    write_status();
}
