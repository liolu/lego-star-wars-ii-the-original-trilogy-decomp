/* La liaison sans fil du DS (bibliotheque WM du SDK), passee par le reseau du PC.
 *
 * Le code du jeu appelle WM_* comme sur la console. Ici ces fonctions remplacent celles du SDK : l'hote ecoute en TCP, l'invite s'y connecte
 * (--net-port n, --net-join adresse[:port]). Le jeu partage ses donnees image par image (WM_StepDataSharing) : l'hote attend l'envoi de chaque
 * invite, puis renvoie le lot complet a tous, dans le meme ordre que sur la console. Sans ces options, personne ne repond et le jeu le voit comme
 * un DS seul. Pas de partie telechargee (MB, une seule carte) : le jeu de l'invite serait un autre programme, Multiboot/child.bin. */
#include "nitro.h"

typedef u64 (*Fn)();

enum { WM_OK = 0, WM_FAILED = 1, WM_OPERATING = 2, WM_ILLEGAL = 3, WM_NO_DATASET = 5, WM_NO_CHILD = 7 };
enum { ST_PARENT_START = 0, ST_SCAN_START = 3, ST_NOT_FOUND = 4, ST_FOUND = 5, ST_CONNECT_START = 6, ST_CONNECTED = 7, ST_DISCONNECTED = 9, ST_MP_START = 10 };
/* MSG_MODS : empreinte du runtime et des mods (nds_mods_fingerprint) ; deux joueurs differents se desynchroniseraient */
enum { MSG_INFO = 1, MSG_CONNECT, MSG_WELCOME, MSG_STEP, MSG_SET, MSG_MODS };

#define NO_SOCKET OS_NO_SOCKET
#define MAX_PEERS 4 /* aid 1 a 3 chez l'hote ; l'hote lui-meme chez l'invite */
#define BUF_SIZE 2048
#define STEPS 16
#define DS_MAX 256
#define BSS_SIZE 0xC0
#define SCAN_TIME 12 /* images d'attente d'une reponse pour un balayage */

typedef struct {
    u32 sock;
    u8 buf[BUF_SIZE];
    u32 have;
    int joined;
    int same; /* meme empreinte que nous (MSG_MODS) */
    u8 steps[STEPS][DS_MAX];
    u32 step_r, step_w;
} Peer;

static Peer peers[MAX_PEERS];
static u32 listener = NO_SOCKET;
static int ready, is_parent, my_aid;
static u32 net_port;
static const char *join_host;

static u32 parent_cb, scan_cb, connect_cb;
static u8 parent_param[0x40];
static u8 own_bss[BSS_SIZE], host_bss[BSS_SIZE];
static int have_host_bss;
static u32 scan_end;
static u32 scan_param;

/* partage de donnees */
static int ds_on;
static u32 ds_len;
static u32 ds_seq;
static int own_stored;
static u8 own_data[DS_MAX];
static u8 set_buf[STEPS][4 + 4 * DS_MAX];
static u32 set_r, set_w;

/* ------------------------------------------------------------------ rappels : dans le contexte d'une interruption, comme sur la console */
typedef struct { u32 cb; u16 data[0x80]; } Pending;
#define QUEUE 32
static Pending queue[QUEUE];
static u32 queue_r, queue_w;

static u16 *post(u32 cb, u16 api, u16 err) {
    static u16 discard[0x80];
    if (!cb || queue_w - queue_r >= QUEUE) return discard;
    Pending *p = &queue[queue_w++ % QUEUE];
    p->cb = cb;
    nds_zero(p->data, sizeof p->data);
    p->data[0] = api;
    p->data[1] = err;
    if (nds_arg("--net-log", 0)) nds_log("wm %u err %u image %u", api, err, nds_frame);
    return p->data;
}

static void post_state(u32 cb, u16 api, u16 state) { post(cb, api, WM_OK)[4] = state; }

/* ------------------------------------------------------------------ sockets */
static void net_start(void) {
    if (ready) return;
    ready = 1;
    const char *value;
    if (nds_arg("--net-port", &value) && value) while (*value) net_port = net_port * 10 + (u32)(*value++ - '0');
    nds_arg("--net-join", &join_host);
    for (int i = 0; i < MAX_PEERS; i++) peers[i].sock = NO_SOCKET;
}

static void peer_close(Peer *p) {
    if (p->sock != NO_SOCKET) os_tcp_close(p->sock);
    p->sock = NO_SOCKET;
    p->have = 0;
    p->joined = 0;
    p->same = 0;
    p->step_r = p->step_w = 0;
}

static void peer_send(Peer *p, u8 type, const void *a, u32 a_len, const void *b, u32 b_len) {
    if (p->sock == NO_SOCKET) return;
    u8 head[3];
    u32 size = a_len + b_len;
    head[0] = (u8)size;
    head[1] = (u8)(size >> 8);
    head[2] = type;
    /* peu d'octets a chaque fois : le tampon du systeme n'a pas le temps de se remplir */
    os_tcp_send(p->sock, head, 3);
    if (a_len) os_tcp_send(p->sock, a, a_len);
    if (b_len) os_tcp_send(p->sock, b, b_len);
}

static void close_all(void) {
    for (int i = 0; i < MAX_PEERS; i++) peer_close(&peers[i]);
    if (listener != NO_SOCKET) os_tcp_close(listener);
    listener = NO_SOCKET;
    is_parent = 0;
    ds_on = 0;
    own_stored = 0;
    have_host_bss = 0;
    set_r = set_w = 0;
    scan_cb = 0;
}

/* ------------------------------------------------------------------ messages */
static void child_lost(int reason) {
    u16 *d = post(connect_cb, 12, WM_OK);
    d[4] = ST_DISCONNECTED;
    d[5] = (u16)my_aid;
    d[6] = (u16)reason;
    my_aid = 0;
    nds_log("reseau : hote perdu");
}

static void parent_lost(int aid) {
    u16 *d = post(parent_cb, 8, WM_OK);
    d[4] = ST_DISCONNECTED;
    d[8] = (u16)aid;
    d[9] = 1;
    nds_log("reseau : invite %d perdu", aid);
}

static u32 text_len(const char *s) {
    u32 n = 0;
    while (s[n]) n++;
    return n;
}

static void send_fingerprint(Peer *p) {
    const char *mine = nds_mods_fingerprint();
    peer_send(p, MSG_MODS, mine, text_len(mine), 0, 0);
}

/* l'autre joueur n'a pas le meme runtime ou les memes mods : la partie est refusee des deux cotes */
static int check_fingerprint(Peer *p, const u8 *data, u32 size) {
    const char *mine = nds_mods_fingerprint();
    u32 n = text_len(mine);
    int same = size == n;
    for (u32 i = 0; same && i < n; i++) same = data[i] == (u8)mine[i];
    if (same) return 1;
    char theirs[512];
    u32 k = size < sizeof theirs - 1 ? size : sizeof theirs - 1;
    nds_copy(theirs, data, k);
    theirs[k] = 0;
    nds_log("reseau : partie refusee, ici [%s], la-bas [%s]", mine, theirs);
    int fr = present_language()[0] == 'f';
    present_notice(fr ? "Partie refusee : l'autre joueur n'a pas la meme version du jeu ou les memes mods."
                      : "Game refused: the other player has another version of the game or other mods.", 6000);
    peer_close(p);
    return 0;
}

static void handle(Peer *p, int aid, u8 type, const u8 *data, u32 size) {
    switch (type) {
    case MSG_MODS:
        if (!check_fingerprint(p, data, size)) return;
        if (!is_parent && !p->same) send_fingerprint(p); /* l'invite repond : l'hote verifie aussi */
        p->same = 1;
        break;
    case MSG_INFO:
        if (size == BSS_SIZE) {
            nds_copy(host_bss, data, BSS_SIZE);
            have_host_bss = 1;
        }
        break;
    case MSG_CONNECT:
        if (is_parent && !p->same) {
            nds_log("reseau : invite %d refuse, sans la meme empreinte", aid);
            peer_close(p);
            return;
        }
        if (is_parent && !p->joined) {
            u16 welcome = (u16)aid;
            p->joined = 1;
            peer_send(p, MSG_WELCOME, &welcome, 2, 0, 0);
            u16 *d = post(parent_cb, 8, WM_OK);
            d[4] = ST_CONNECTED;
            ((u8 *)d)[10] = 0x00; ((u8 *)d)[11] = 0x09; ((u8 *)d)[12] = 0xBF; ((u8 *)d)[15] = (u8)aid;
            d[8] = (u16)aid;
            nds_log("reseau : invite %d connecte", aid);
        }
        break;
    case MSG_WELCOME:
        my_aid = data[0] | data[1] << 8;
        {
            u16 *d = post(connect_cb, 12, WM_OK);
            d[4] = ST_CONNECTED;
            d[5] = (u16)my_aid;
        }
        nds_log("reseau : connecte, numero %d", my_aid);
        break;
    case MSG_STEP:
        if (size <= DS_MAX && p->step_w - p->step_r < STEPS) nds_copy(p->steps[p->step_w++ % STEPS], data, size);
        break;
    case MSG_SET:
        if (size <= sizeof set_buf[0] && set_w - set_r < STEPS) nds_copy(set_buf[set_w++ % STEPS], data, size);
        break;
    }
}

static void peer_read(Peer *p, int aid) {
    if (p->sock == NO_SOCKET) return;
    for (;;) {
        int got = os_tcp_recv(p->sock, p->buf + p->have, BUF_SIZE - p->have);
        if (got == 0) {
            int was_joined = p->joined || (!is_parent && my_aid);
            peer_close(p);
            if (was_joined) {
                if (is_parent) parent_lost(aid);
                else child_lost(1);
            }
            return;
        }
        if (got < 0) return;
        p->have += (u32)got;
        u32 at = 0;
        while (p->have - at >= 3) {
            u32 size = p->buf[at] | (u32)p->buf[at + 1] << 8;
            if (p->have - at < 3 + size) break;
            handle(p, aid, p->buf[at + 2], p->buf + at + 3, size);
            if (p->sock == NO_SOCKET) return;
            at += 3 + size;
        }
        for (u32 i = at; i < p->have; i++) p->buf[i - at] = p->buf[i];
        p->have -= at;
    }
}

static void net_pump(void) {
    if (!ready) return;
    if (listener != NO_SOCKET) {
        u32 s = os_tcp_accept(listener);
        if (s != NO_SOCKET) {
            int placed = 0;
            for (int aid = 1; aid < MAX_PEERS && !placed; aid++) {
                if (peers[aid].sock != NO_SOCKET) continue;
                peers[aid].sock = s;
                peer_send(&peers[aid], MSG_INFO, own_bss, BSS_SIZE, 0, 0);
                send_fingerprint(&peers[aid]);
                placed = 1;
            }
            if (!placed) os_tcp_close(s);
        }
    }
    for (int i = 0; i < MAX_PEERS; i++) peer_read(&peers[i], i);
}

/* Le menu multijoueur du jeu s'ouvre avec Select + X (--net-port ou --net-join), sans passer par l'arene du jeu. */
extern const u32 nds_net_screens, nds_net_menu_screen;
extern void *const nds_open_screen;

static void menu_shortcut(void) {
    static int was_down, first = 1, active;
    if (first) {
        first = 0;
        active = nds_arg("--net-port", 0) || nds_arg("--net-join", 0) || nds_arg("--net-menu", 0);
    }
    if (!active || !nds_open_screen) return;
    u32 keys = present_keys();
    u32 xy = REG16(0x027FFFA8u);
    int down = (keys & 4) && !(xy & (1u << 10));
    const char *at;
    if (nds_arg("--net-menu", &at) && at) {
        u32 frame = 0;
        while (*at) frame = frame * 10 + (u32)(*at++ - '0');
        down = nds_frame == frame;
    }
    if (down && !was_down) {
        nds_log("reseau : menu multijoueur");
        ((void (*)(u32, u32, u32, u32, u32, u32))nds_open_screen)(nds_net_screens, nds_net_menu_screen, 1, 0, 1, 0);
    }
    was_down = down;
}

/* appele par irq_deliver, comme l'interruption de la FIFO du processeur ARM7 sur la console */
void net_irq(void) {
    menu_shortcut();
    if (!ready) return;
    net_pump();
    if (nds_frame % 300 == 0 && nds_arg("--net-log", 0)) {
        static u32 last = 0xFFFFFFFFu;
        if (last != nds_frame) nds_log("image %u : partage %d, lot %u, en attente %u", nds_frame, ds_on, ds_seq, peers[1].step_w - peers[1].step_r);
        last = nds_frame;
    }
    if (scan_cb && scan_param) {
        if (have_host_bss) {
            u32 out = REG32(scan_param);
            if (out) nds_copy(DS(out), host_bss, BSS_SIZE);
            u8 *d = (u8 *)post(scan_cb, 10, WM_OK);
            ((u16 *)d)[4] = ST_FOUND;
            nds_copy(d + 0xA, host_bss + 4, 6);
            *(u16 *)(d + 0x10) = *(u16 *)(host_bss + 0x36);
            *(u16 *)(d + 0x12) = 3;
            *(u16 *)(d + 0x14) = *(u16 *)(host_bss + 0xA);
            nds_copy(d + 0x16, host_bss + 0xC, 32);
            *(u16 *)(d + 0x36) = *(u16 *)(host_bss + 0x3C);
            nds_copy(d + 0x38, host_bss + 0x40, 0x80);
            scan_param = 0;
        } else if ((s32)(nds_frame - scan_end) >= 0) {
            post_state(scan_cb, 10, ST_NOT_FOUND);
            scan_param = 0;
        }
    }
    /* un rappel peut en poster d'autres : ils attendent l'interruption suivante */
    u32 end = queue_w;
    while (queue_r != end) {
        Pending *p = &queue[queue_r % QUEUE];
        queue_r++;
        if (nds_arg("--net-log", 0)) nds_log("rappel api %u etat %u", p->data[0], p->data[4]);
        /* le jeu lit ce message a une adresse DS : il passe par la pile, qui est dans la memoire DS (la file ne l'est pas) */
        u16 data[0x80] __attribute__((aligned(4)));
        nds_copy(data, p->data, sizeof data);
        ((Fn)nds_call_target(p->cb))(HOST2DS(data), 0, 0, 0);
    }
}

/* ------------------------------------------------------------------ WM : demarrage et arret */
u32 WM_Init(u32 buf, u32 dma) { net_start(); return WM_OK; }
u32 WM_Enable(u32 cb) { net_start(); post(cb, 3, WM_OK); return WM_OPERATING; }
u32 WM_PowerOn(u32 cb) { post(cb, 5, WM_OK); return WM_OPERATING; }
u32 WM_Reset(u32 cb) { close_all(); post(cb, 1, WM_OK); return WM_OPERATING; }
u32 WM_End(u32 cb) { close_all(); post(cb, 2, WM_OK); return WM_OPERATING; }
/* la partie telechargee n'est pas prise en charge : le jeu l'apprend ici et revient a son menu */
u32 WM_Initialize(u32 buf, u32 cb, u32 dma) { return WM_FAILED; }
u32 WM_GetAllowedChannel(void) { return 0x2082; }
u32 WM_GetLinkLevel(void) { return 3; }
u32 WM_GetDispersionBeaconPeriod(void) { return 200; }
u32 WM_GetDispersionScanPeriod(void) { return 30; }
u32 WM_SetEntry(u32 enabled) { return WM_OPERATING; }

u32 WM_MeasureChannel(u32 cb, u32 mode, u32 threshold, u32 channel, u32 time) {
    u16 *d = post(cb, 40, WM_OK);
    d[4] = (u16)channel;
    d[5] = 0;
    return WM_OPERATING;
}

/* ------------------------------------------------------------------ hote */
u32 WM_SetParentParameter(u32 cb, u32 param) {
    nds_copy(parent_param, DS(param), sizeof parent_param);
    post(cb, 7, WM_OK);
    return WM_OPERATING;
}

static void make_own_bss(void) {
    const u8 *p = parent_param;
    u8 *b = own_bss;
    u32 user = *(const u32 *)p;
    u32 user_len = *(const u16 *)(p + 4);
    if (user_len > 112) user_len = 112;
    nds_zero(b, BSS_SIZE);
    *(u16 *)b = BSS_SIZE / 2;
    b[5] = 0x09; b[6] = 0xBF; /* adresse de l'hote */
    *(u16 *)(b + 0xA) = 32;
    nds_copy(b + 0xC, p + 8, 4); /* ggid */
    nds_copy(b + 0x10, p + 0xC, 2); /* tgid */
    *(u16 *)(b + 0x32) = *(const u16 *)(p + 0x18);
    *(u16 *)(b + 0x36) = *(const u16 *)(p + 0x32);
    *(u16 *)(b + 0x3C) = (u16)(0x10 + user_len);
    u8 *info = b + 0x40;
    *(u16 *)info = 1;
    info[2] = 1;
    nds_copy(info + 4, p + 8, 4);
    nds_copy(info + 8, p + 0xC, 2);
    info[0xA] = (u8)user_len;
    info[0xB] = (u8)((*(const u16 *)(p + 0xE) & 1) | (*(const u16 *)(p + 0x12) & 1) << 1 | (*(const u16 *)(p + 0x14) & 1) << 2 | (*(const u16 *)(p + 0x16) & 1) << 3);
    *(u16 *)(info + 0xC) = *(const u16 *)(p + 0x34);
    *(u16 *)(info + 0xE) = *(const u16 *)(p + 0x36);
    if (user && user_len) nds_copy(info + 0x10, DS(user), user_len);
}

u32 WM_StartParent(u32 cb) {
    net_start();
    parent_cb = cb;
    is_parent = 1;
    my_aid = 0;
    make_own_bss();
    post_state(cb, 8, ST_PARENT_START);
    if (net_port && listener == NO_SOCKET) {
        listener = os_tcp_listen(net_port);
        if (listener == NO_SOCKET) nds_log("reseau : le port %u n'a pas pu etre ouvert", net_port);
        else nds_log("reseau : hote sur le port %u", net_port);
    }
    return WM_OPERATING;
}

u32 WM_EndParent(u32 cb) {
    close_all();
    post(cb, 9, WM_OK);
    return WM_OPERATING;
}

/* ------------------------------------------------------------------ invite */
static void try_connect(void) {
    if (!join_host || peers[0].sock != NO_SOCKET) return;
    char name[128];
    u32 n = 0, port = net_port ? net_port : 27016;
    const char *t = join_host;
    while (*t && *t != ':' && n < sizeof name - 1) name[n++] = *t++;
    name[n] = 0;
    if (*t == ':') {
        port = 0;
        for (t++; *t >= '0' && *t <= '9'; t++) port = port * 10 + (u32)(*t - '0');
    }
    u32 s = os_tcp_connect(name, port);
    if (s == NO_SOCKET) return;
    peers[0].sock = s;
    peers[0].have = 0;
}

u32 WM_StartScan(u32 cb, u32 param) {
    net_start();
    scan_cb = cb;
    scan_param = param;
    scan_end = nds_frame + SCAN_TIME;
    have_host_bss = have_host_bss && peers[0].sock != NO_SOCKET;
    post_state(cb, 10, ST_SCAN_START);
    try_connect();
    return WM_OPERATING;
}

u32 WM_EndScan(u32 cb) {
    scan_cb = 0;
    scan_param = 0;
    post(cb, 11, WM_OK);
    return WM_OPERATING;
}

u32 WM_StartConnectEx(u32 cb, u32 bss, u32 ssid, u32 power_save, u32 auth) {
    connect_cb = cb;
    if (peers[0].sock == NO_SOCKET) return WM_FAILED;
    post_state(cb, 12, ST_CONNECT_START);
    peer_send(&peers[0], MSG_CONNECT, 0, 0, 0, 0);
    return WM_OPERATING;
}

u32 WM_Disconnect(u32 cb, u32 aid) {
    if (is_parent) {
        if (aid < MAX_PEERS) {
            int was = peers[aid].joined;
            peer_close(&peers[aid]);
            if (was) parent_lost((int)aid);
        }
    } else if (my_aid) {
        peer_close(&peers[0]);
        child_lost(2);
    }
    post(cb, 13, WM_OK);
    return WM_OPERATING;
}

/* ------------------------------------------------------------------ echanges */
u32 WM_StartMP(u32 cb, u32 recv_buf, u32 recv_size, u32 send_buf, u32 send_size, u32 freq) {
    u16 *d = post(cb, 14, WM_OK);
    d[2] = ST_MP_START;
    return WM_OPERATING;
}

u32 WM_EndMP(u32 cb) {
    post(cb, 16, WM_OK);
    return WM_OPERATING;
}

u32 WM_StartDataSharing(u32 info, u32 port, u32 bitmap, u32 length, u32 twice) {
    if (length > DS_MAX) return WM_FAILED;
    ds_on = 1;
    ds_len = length;
    ds_seq = 0;
    own_stored = 0;
    return WM_OK;
}

u32 WM_EndDataSharing(u32 info) {
    ds_on = 0;
    return WM_OK;
}

#define SET_BITMAP(set) (*(u16 *)(set))
#define SET_SLOT(set, aid) ((u8 *)(set) + 4 + (u32)(aid) * ds_len)

u32 WM_StepDataSharing(u32 info, u32 send_data, u32 receive) {
    if (!ds_on) return WM_ILLEGAL;
    net_pump();
    if (is_parent) {
        u32 joined = 0;
        for (int aid = 1; aid < MAX_PEERS; aid++) if (peers[aid].joined) joined |= 1u << aid;
        if (!joined) return WM_NO_CHILD;
        if (!own_stored) {
            nds_copy(own_data, DS(send_data), ds_len);
            own_stored = 1;
        }
        for (int aid = 1; aid < MAX_PEERS; aid++)
            if ((joined >> aid & 1) && peers[aid].step_r == peers[aid].step_w) return WM_NO_DATASET;
        u8 *set = DS(receive);
        SET_BITMAP(set) = (u16)(joined | 1);
        nds_zero(set + 4, 4 * ds_len);
        nds_copy(SET_SLOT(set, 0), own_data, ds_len);
        for (int aid = 1; aid < MAX_PEERS; aid++) {
            if (!(joined >> aid & 1)) continue;
            nds_copy(SET_SLOT(set, aid), peers[aid].steps[peers[aid].step_r++ % STEPS], ds_len);
        }
        for (int aid = 1; aid < MAX_PEERS; aid++)
            if (joined >> aid & 1) peer_send(&peers[aid], MSG_SET, set, 4 + 4 * ds_len, 0, 0);
        own_stored = 0;
        ds_seq++;
        return WM_OK;
    }
    if (!my_aid) return WM_ILLEGAL;
    if (!own_stored) {
        peer_send(&peers[0], MSG_STEP, DS(send_data), ds_len, 0, 0);
        own_stored = 1;
    }
    if (set_r == set_w) return WM_NO_DATASET;
    nds_copy(DS(receive), set_buf[set_r++ % STEPS], 4 + 4 * ds_len);
    own_stored = 0;
    ds_seq++;
    return WM_OK;
}

u32 WM_GetSharedDataAddress(u32 info, u32 receive, u32 aid) {
    u8 *set = DS(receive);
    if (aid >= MAX_PEERS || !(SET_BITMAP(set) >> aid & 1)) return 0;
    return receive + 4 + aid * ds_len;
}
