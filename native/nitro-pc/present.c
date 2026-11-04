/* Sortie : les deux ecrans vers le launcher (--shm), ou images BMP pour les essais (--dump dossier --every n --frames n).
 * Definition : --scale k (pixels par pixel DS), --dswidth n (largeur en pixels DS, 256 = 4:3, 341 = 16:9). */
#include "nitro.h"

static const char *dump_dir;
static u32 every = 60, max_frames, dump_from;
static int pipe_mode;
static const char *key_script;
static void input_thread(void *arg);
static volatile u32 keys_now;
static volatile int touch_down, touch_x, touch_y;
static void script_step(void);
static void peek_step(void);
static void look_step(void);
static void hid_step(void);
static u8 *shm;
static volatile int cfg_scale = 1, cfg_width = 256;
static void header_init(void);
static int lang_fr;

void present_init(void) {
    const char *v;
    nds_arg("--dump", &dump_dir);
    if (nds_arg("--every", &v) && v) { every = 0; while (*v) every = every * 10 + *v++ - '0'; }
    if (nds_arg("--frames", &v) && v) { while (*v) max_frames = max_frames * 10 + *v++ - '0'; }
    if (nds_arg("--dumpfrom", &v) && v) { while (*v) dump_from = dump_from * 10 + *v++ - '0'; }
    nds_arg("--keys", &key_script);
    if (nds_arg("--scale", &v) && v) { cfg_scale = 0; while (*v) cfg_scale = cfg_scale * 10 + *v++ - '0'; }
    if (nds_arg("--dswidth", &v) && v) { cfg_width = 0; while (*v) cfg_width = cfg_width * 10 + *v++ - '0'; }
    if (nds_arg("--shm", &v) && v) {
        shm = os_shm_open(v);
        if (!shm) nds_fatal("memoire d'images introuvable : %s", v);
    }
    if (nds_arg("--lang", &v) && v) lang_fr = v[0] == 'f';
    header_init();
    pipe_mode = nds_arg("--pipe", 0);
    if (pipe_mode) os_thread_start(input_thread, 0, 0x10000);
}

/* Images partagees avec le launcher : en-tete, puis deux jeux de deux ecrans, RGBA 8 bits. */
#define MAX_W 2688
#define MAX_H 1152
#define SHM_HEADER 4096
/* En-tete de la memoire partagee (native_game.rs) : le module y ecrit ses images, et ce qu'il demande au launcher.
 * protocol 2 : le launcher envoie alors le paquet de 64 octets (input_thread). flags : bit 0 = retenir le curseur.
 * notice_seq change a chaque message ; le texte (UTF-8, termine par 0) est a NOTICE_AT. */
typedef struct { volatile u32 seq, width, height, front, protocol, flags, notice_seq, notice_ms; } ShmHeader;
#define PROTOCOL 2
#define NOTICE_AT 64
#define NOTICE_MAX 960
static u32 *local_frames;
static int back_set;
static volatile u32 flips;

static u32 *frame_ptr(int set, int screen) {
    u8 *base = shm ? shm + SHM_HEADER : (u8 *)local_frames;
    return (u32 *)(base + (u32)(set * 2 + screen) * MAX_W * MAX_H * 4);
}

void present_config(int *scale, int *ds_width) {
    int k = cfg_scale, w = cfg_width;
    if (k < 1) k = 1;
    if (k > 6) k = 6;
    if (w < 256) w = 256;
    if (w * k > MAX_W) w = MAX_W / k;
    *scale = k;
    *ds_width = w;
}

void present_back(u32 **top, u32 **bottom) {
    if (!shm && !local_frames) local_frames = os_mem_alloc(0, 4u * MAX_W * MAX_H * 4);
    *top = frame_ptr(back_set, 0);
    *bottom = frame_ptr(back_set, 1);
}

static void write_bmp(u32 frame, const u32 *top, const u32 *bottom, int w, int h) {
    static u8 *file;
    if (!file) file = os_mem_alloc(0, 54 + MAX_W * MAX_H * 2 * 3 + MAX_H * 8);
    int stride = (w * 3 + 3) & ~3;
    u32 size = 54 + stride * h * 2;
    u8 *d = file;
    nds_zero(d, 54);
    d[0] = 'B'; d[1] = 'M';
    *(u32 *)(d + 2) = size; *(u32 *)(d + 10) = 54; *(u32 *)(d + 14) = 40;
    *(u32 *)(d + 18) = w; *(u32 *)(d + 22) = h * 2;
    *(u16 *)(d + 26) = 1; *(u16 *)(d + 28) = 24;
    u8 *p = d + 54;
    for (int y = h * 2 - 1; y >= 0; y--) {
        const u32 *line = y < h ? top + y * MAX_W : bottom + (y - h) * MAX_W;
        for (int x = 0; x < w; x++) {
            u32 c = line[x];
            p[x * 3] = (u8)(c >> 16);
            p[x * 3 + 1] = (u8)(c >> 8);
            p[x * 3 + 2] = (u8)c;
        }
        p += stride;
    }
    char name[300];
    int n = 0;
    for (const char *t = dump_dir; *t && n < 260; t++) name[n++] = *t;
    name[n++] = '/';
    u32 v = frame, div = 100000;
    while (div) { name[n++] = '0' + (v / div) % 10; div /= 10; }
    name[n++] = '.'; name[n++] = 'b'; name[n++] = 'm'; name[n++] = 'p'; name[n] = 0;
    os_file_save(name, file, size);
}

/* Fin d'une image dans le fil de rendu : les lignes vont d'un seul tenant dans le jeu d'images, et le launcher lit le jeu termine */
void present_flip(int w, int h, u32 frame) {
    static u32 last_dump = 0xFFFFFFFFu;
    if (nds_cb.present) nds_mods_present(frame_ptr(back_set, 0), frame_ptr(back_set, 1), w, h, MAX_W);
    if (dump_dir && frame >= dump_from && frame / every != last_dump) {
        last_dump = frame / every;
        write_bmp(frame, frame_ptr(back_set, 0), frame_ptr(back_set, 1), w, h);
    }
    if (shm) {
        ShmHeader *header = (ShmHeader *)shm;
        header->width = w;
        header->height = h;
        header->front = back_set;
        header->seq++;
    }
    back_set = !back_set;
    flips++;
}

extern int nds_trace;
extern u32 nds_trace_count, nds_trace_eip, nds_trace_addr, nds_trace_stack[13];

void present_frame(void) {
    script_step();
    peek_step();
    look_step();
    hid_step();
    /* boutons X/Y : actifs a 0 ; bit 15 a 0 = couvercle ouvert */
    {
        u32 xy = 0x2C00; /* X = bit 10, Y = bit 11, debogage = bit 13 : relaches a 1, comme KEYINPUT */
        u32 for_game = nds_mods_input_taken ? 0 : keys_now; /* un mod qui prend les touches les garde */
        if (for_game & (1 << 10)) xy &= ~(1u << 10);
        if (for_game & (1 << 11)) xy &= ~(1u << 11);
        REG16(0x027FFFA8u) = (u16)xy;
        /* dernier releve du stylet pour le SDK : x 12 bits, y 12 bits, touche */
        REG16(0x027FFFAAu) = (u16)((touch_x & 0xFFF) | ((touch_y & 0xF) << 12));
        REG16(0x027FFFACu) = (u16)(((touch_y >> 4) & 0xFF) | (touch_down ? 0x100 : 0));
    }
    static int trace_on = -1;
    if (trace_on < 0) trace_on = nds_arg("--trace", 0);
    if (trace_on) {
        nds_trace = 1;
        if (nds_frame % every == 0) {
            nds_log("image %u : %u acces registres, dernier %x eip=%x", nds_frame, nds_trace_count, nds_trace_addr, nds_trace_eip);
            nds_trace_count = 0;
            nds_log("  dispA=%x dispB=%x brA=%x brB=%x pow=%x vram=%x %x bg0A=%x", REG32(IO), REG32(IO + 0x1000), REG16(IO + 0x6C), REG16(IO + 0x106C), REG16(IO + 0x304), REG32(IO + 0x240), REG32(IO + 0x244), REG16(IO + 8));
            {
                u32 pal = 0, pal2 = 0, vram = 0, oam = 0;
                for (u32 i = 0; i < 0x200; i += 2) { if (REG16(0x05000000u + i)) pal++; if (REG16(0x05000400u + i)) pal2++; }
                for (u32 i = 0; i < 0x80000; i += 4) if (REG32(0x06000000u + i)) vram++;
                for (u32 i = 0; i < 0x400; i += 8) if ((REG16(0x07000000u + i) & 0x300) != 0x200) oam++;
                nds_log("  pal=%u palB=%u vramBG=%u objs=%u polys=%u etat=%x sub=%x clear=%x disp3d=%x", pal, pal2, vram, oam, REG16(IO + 0x604), REG32(0x02088438u), REG32(0x02088448u), REG32(IO + 0x350), REG16(IO + 0x60));
            }
            if (nds_arg("--threads-dump", 0)) { extern void nds_dump_threads(void); nds_dump_threads(); }
            nds_log("    %8x", nds_trace_eip);
            for (int i = 0; nds_trace_stack[i]; i++) nds_log("    %8x", nds_trace_stack[i]);
        }
    }
#ifdef NDS_INSTRUMENT
    if (nds_arg("--ring", 0) && nds_frame % every == every - 1) {
        extern u32 nds_ring[4096], nds_ring_pos;
        nds_log("appels avant l'image %u :", nds_frame);
        for (int i = 0; i < 4096; i++) nds_log("    %8x", nds_ring[(nds_ring_pos + i) & 4095]);
    }
#endif
    if (max_frames && nds_frame >= max_frames) {
        extern u32 nds_swaps;
        nds_log("%u images, %u dessinees, %u echanges 3D", nds_frame, flips, nds_swaps);
        os_exit(0);
    }
}

/* commandes du launcher, a chaque changement. Paquet 1 (16 octets, launchers d'avant le protocole 2) : touches du DS,
 * stylet, definition, cumul de la souris. Paquet 2 (64 octets, commence par 0x8002, que les touches du paquet 1 n'atteignent
 * jamais) : la meme chose, puis boutons et molette de la souris, etat de la fenetre et clavier entier (codes HID). */
static volatile u32 keys_now;
static volatile int touch_down, touch_x, touch_y;
static volatile int mouse_x, mouse_y;
static volatile u32 mouse_buttons, window_flags, keyboard[8];
static volatile int wheel;

void present_notice(const char *text, u32 ms) {
    if (!shm) { nds_log("message : %s", text); return; }
    ShmHeader *h = (ShmHeader *)shm;
    u8 *at = shm + NOTICE_AT;
    u32 n = 0;
    while (text[n] && n < NOTICE_MAX - 1) { at[n] = (u8)text[n]; n++; }
    at[n] = 0;
    h->notice_ms = ms;
    __sync_synchronize();
    h->notice_seq++;
}

static void header_init(void) {
    if (shm) ((ShmHeader *)shm)->protocol = PROTOCOL;
}

/* le curseur retenu : un mod le demande (api->capture_mouse), par exemple pour diriger une camera a la souris */
static void header_flags(void) {
    if (shm) ((ShmHeader *)shm)->flags = nds_mods_mouse_captured ? 1 : 0;
}

static void read_all(void *buf, u32 size) {
    u32 have = 0;
    while (have < size) {
        u32 got = os_stdin_read((u8 *)buf + have, size - have);
        if (!got) os_exit(0); /* launcher ferme */
        have += got;
    }
}

static void input_thread(void *arg) {
    u16 packet[32];
    for (;;) {
        read_all(packet, 16);
        if (packet[0] == 0x8002) {
            /* mots de 16 bits : 0 = 0x8002, 1 touches, 2-4 stylet, 5 largeur, 6 echelle, 7-8 cumul de la souris,
             * 9 fenetre (bit 0 curseur retenu, bit 1 active), 10-11 boutons, 12-13 molette, 16-31 clavier (256 bits) */
            read_all(packet + 8, 48);
            nds_copy((void *)keyboard, packet + 16, 32);
            keys_now = packet[1] & 0xFFF;
            touch_down = packet[2];
            touch_x = packet[3];
            touch_y = packet[4];
            cfg_width = packet[5];
            cfg_scale = packet[6];
            mouse_x = (s16)packet[7];
            mouse_y = (s16)packet[8];
            window_flags = packet[9];
            mouse_buttons = packet[10] | (u32)packet[11] << 16;
            wheel = (s32)(packet[12] | (u32)packet[13] << 16);
            continue;
        }
        keys_now = packet[0];
        touch_down = packet[1];
        touch_x = packet[2];
        touch_y = packet[3];
        cfg_width = packet[4];
        cfg_scale = packet[5];
        mouse_x = (s16)packet[6];
        mouse_y = (s16)packet[7];
    }
}

/* essais sans fenetre : --keys "600:8,620:0" = a l'image 600 le masque de touches 8 (Start), etc. */

static void script_step(void) {
    while (key_script && *key_script) {
        u32 frame = 0, mask = 0;
        const char *p = key_script;
        while (*p >= '0' && *p <= '9') frame = frame * 10 + *p++ - '0';
        if (frame > nds_frame) return;
        if (*p == ':') p++;
        while (*p >= '0' && *p <= '9') mask = mask * 10 + *p++ - '0';
        keys_now = mask & 0xFFF;
        if (*p == '/') { /* image:masque/x/y : stylet */
            int x = 0, y = 0;
            p++;
            while (*p >= '0' && *p <= '9') x = x * 10 + *p++ - '0';
            if (*p == '/') p++;
            while (*p >= '0' && *p <= '9') y = y * 10 + *p++ - '0';
            touch_down = 1;
            touch_x = x;
            touch_y = y;
        } else {
            touch_down = 0;
        }
        if (*p == ',') p++;
        key_script = p;
    }
}

/* essais : --hid "image:code,..." tient une touche du clavier (code HID, NDS_KEY_* de sdk/nds_mod.h), 0 = toutes relachees ;
 * pour les mods qui lisent le clavier (api->input) */
static void hid_step(void) {
    static const char *hids;
    static int ready;
    if (!ready) {
        ready = 1;
        nds_arg("--hid", &hids);
    }
    while (hids && *hids) {
        const char *p = hids;
        u32 frame = 0, code = 0;
        while (*p >= '0' && *p <= '9') frame = frame * 10 + *p++ - '0';
        if (frame > nds_frame) break;
        if (*p == ':') p++;
        while (*p >= '0' && *p <= '9') code = code * 10 + *p++ - '0';
        for (int i = 0; i < 8; i++) keyboard[i] = 0;
        if (code && code < 256) keyboard[code >> 5] = 1u << (code & 31);
        hids = *p == ',' ? p + 1 : p;
    }
}

/* essais : --look "image:x:y,..." donne le cumul de la souris (entiers positifs) */
static u32 script_number(const char **t, int hex);

static void look_step(void) {
    static const char *looks;
    static int ready;
    if (!ready) {
        ready = 1;
        nds_arg("--look", &looks);
    }
    while (looks && *looks) {
        const char *p = looks;
        u32 frame = script_number(&p, 0);
        if (frame > nds_frame) break;
        mouse_x = (int)script_number(&p, 0);
        mouse_y = (int)script_number(&p, 0);
        looks = *p == ',' ? p + 1 : p;
    }
}

/* essais : --peek "image:adresse:taille:fichier,..." ecrit de la memoire dans un fichier, --poke "image:adresse:valeur,..." y ecrit un mot */
static u32 script_number(const char **t, int hex) {
    u32 v = 0;
    const char *p = *t;
    for (;;) {
        char c = *p;
        if (c >= '0' && c <= '9') v = v * (hex ? 16 : 10) + (u32)(c - '0');
        else if (hex && c >= 'a' && c <= 'f') v = v * 16 + (u32)(c - 'a' + 10);
        else break;
        p++;
    }
    if (*p == ':') p++;
    *t = p;
    return v;
}

static void peek_step(void) {
    static const char *peeks, *pokes;
    static int ready;
    if (!ready) {
        ready = 1;
        nds_arg("--peek", &peeks);
        nds_arg("--poke", &pokes);
    }
    while (pokes && *pokes) {
        const char *p = pokes;
        u32 frame = script_number(&p, 0);
        if (frame > nds_frame) break;
        u32 addr = script_number(&p, 1), value = script_number(&p, 1);
        REG32(addr) = value;
        pokes = *p == ',' ? p + 1 : p;
    }
    while (peeks && *peeks) {
        const char *p = peeks;
        u32 frame = script_number(&p, 0);
        if (frame > nds_frame) break;
        u32 addr = script_number(&p, 1), size = script_number(&p, 1);
        char name[300];
        int n = 0;
        while (*p && *p != ',' && n < 299) name[n++] = *p++;
        name[n] = 0;
        os_file_save(name, DS(addr), size);
        peeks = *p == ',' ? p + 1 : p;
    }
}

/* Touches vues par le jeu : rien si un mod les prend (api->take_input) */
u32 present_keys(void) { return nds_mods_input_taken ? 0 : keys_now & 0x3FF; }

u32 present_pad(void) { return keys_now & 0xFFF; }

void present_input_extra(u32 *buttons, int *wheel_out, u32 *keys) {
    *buttons = mouse_buttons;
    *wheel_out = wheel;
    for (int i = 0; i < 8; i++) keys[i] = keyboard[i];
}

const char *present_language(void) { return lang_fr ? "fr" : "en"; }

u32 present_window_flags(void) { return window_flags; }

void present_flags_changed(void) { header_flags(); }

void present_mouse(int *x, int *y) {
    *x = mouse_x;
    *y = mouse_y;
}

void present_touch(int *down, int *x, int *y) {
    *down = touch_down;
    *x = touch_x;
    *y = touch_y;
}
