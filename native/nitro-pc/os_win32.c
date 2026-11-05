/* Couche systeme pour Windows, 32 ou 64 bits : tous les appels Win32 du runtime sont ici (kernel32.def, winmm.def).
 * DWORD = unsigned long (32 bits dans les deux cas) ; SIZE_T, DWORD_PTR et SOCKET = uptr (taille d'un pointeur). */
#include "nitro.h"
#include "os.h"

#define WINAPI __attribute__((stdcall))
#define IMPORT __attribute__((dllimport))
IMPORT void *WINAPI VirtualAlloc(void *, uptr, unsigned long, unsigned long);
IMPORT int WINAPI VirtualProtect(void *, uptr, unsigned long, unsigned long *);
IMPORT void *WINAPI AddVectoredExceptionHandler(unsigned long, void *);
IMPORT void *WINAPI CreateFileA(const char *, unsigned long, unsigned long, void *, unsigned long, unsigned long, void *);
IMPORT int WINAPI ReadFile(void *, void *, unsigned long, unsigned long *, void *);
IMPORT int WINAPI WriteFile(void *, const void *, unsigned long, unsigned long *, void *);
IMPORT unsigned long WINAPI GetFileSize(void *, unsigned long *);
IMPORT int WINAPI CloseHandle(void *);
IMPORT void *WINAPI GetStdHandle(unsigned long);
IMPORT void WINAPI ExitProcess(unsigned);
IMPORT void WINAPI Sleep(unsigned long);
IMPORT int WINAPI QueryPerformanceCounter(s64 *);
IMPORT int WINAPI QueryPerformanceFrequency(s64 *);
IMPORT const char *WINAPI GetCommandLineA(void);
IMPORT void *WINAPI MapViewOfFile(void *, unsigned long, unsigned long, unsigned long, uptr);
IMPORT void *WINAPI CreateThread(void *, uptr, void *, void *, unsigned long, unsigned long *);
IMPORT int WINAPI SetThreadPriority(void *, int);
IMPORT void *WINAPI GetCurrentThread(void);
IMPORT void *WINAPI OpenFileMappingA(unsigned long, int, const char *);
IMPORT void WINAPI GetLocalTime(void *);
IMPORT unsigned long WINAPI SuspendThread(void *);
IMPORT unsigned long WINAPI ResumeThread(void *);
IMPORT int WINAPI GetThreadContext(void *, void *);
IMPORT int WINAPI DuplicateHandle(void *, void *, void *, void **, unsigned long, int, unsigned long);
IMPORT void *WINAPI GetCurrentProcess(void);
IMPORT void *WINAPI CreateEventA(void *, int, int, const char *);
IMPORT int WINAPI SetEvent(void *);
IMPORT unsigned long WINAPI WaitForSingleObject(void *, unsigned long);
IMPORT unsigned long WINAPI GetActiveProcessorCount(unsigned short);
IMPORT void *WINAPI LoadLibraryA(const char *);
IMPORT void *WINAPI GetProcAddress(void *, const char *);
IMPORT unsigned WINAPI timeBeginPeriod(unsigned);
IMPORT unsigned WINAPI waveOutOpen(void **, unsigned, const void *, uptr, uptr, unsigned long);
IMPORT unsigned WINAPI waveOutPrepareHeader(void *, void *, unsigned);
IMPORT unsigned WINAPI waveOutWrite(void *, void *, unsigned);

#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_COMMIT_RESERVE 0x3000
#define PAGE_NOACCESS 1
#define PAGE_READWRITE 4

/* ------------------------------------------------------------------ bibliotheques */
void *os_lib_open(const char *path) { return LoadLibraryA(path); }
void *os_lib_sym(void *lib, const char *name) { return GetProcAddress(lib, name); }

/* ------------------------------------------------------------------ bornes de la pile (NT_TIB du fil : StackBase, StackLimit) */
static uptr *thread_tib(void) {
    uptr *tib;
#if defined(__x86_64__)
    __asm__ volatile("movq %%gs:0x30, %0" : "=r"(tib));
#else
    __asm__ volatile("movl %%fs:0x18, %0" : "=r"(tib));
#endif
    return tib;
}

void os_stack_bounds(uptr *lo, uptr *hi) {
    uptr *tib = thread_tib();
    *hi = tib[1];
    *lo = tib[2];
}

void os_set_stack_bounds(uptr lo, uptr hi) {
    uptr *tib = thread_tib();
    tib[1] = hi;
    tib[2] = lo;
}

/* ------------------------------------------------------------------ memoire */
#if !NDS_IDENTITY
/* 64 bits : la memoire DS est dans une fenetre reservee (comme sur Mac, os_posix.c) ; os_mem_alloc y engage les zones */
#define WINDOW_SIZE (0x100000000ull + 0x10000u)
static u8 *window;

void *os_mem_window(void) {
    if (!window) window = VirtualAlloc(0, WINDOW_SIZE, MEM_RESERVE, PAGE_NOACCESS);
    return window;
}
#endif

void *os_mem_alloc(void *want, u32 size) {
#if !NDS_IDENTITY
    if (want && window && (u8 *)want >= window && (u8 *)want - window < WINDOW_SIZE)
        return VirtualAlloc(want, size, MEM_COMMIT, PAGE_READWRITE);
#endif
    return VirtualAlloc(want, size, MEM_COMMIT_RESERVE, PAGE_READWRITE);
}

void os_mem_protect(void *p, u32 size, int prot) {
    unsigned long old;
    VirtualProtect(p, size, prot == OS_RW ? PAGE_READWRITE : PAGE_NOACCESS, &old);
}

/* ------------------------------------------------------------------ fichiers */
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3

void *os_file_load(const char *path, u32 *size) {
    void *h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == OS_BAD_FILE) return 0;
    u32 n = GetFileSize(h, 0);
    u8 *data = os_mem_alloc(0, n);
    unsigned long got;
    ReadFile(h, data, n, &got, 0);
    CloseHandle(h);
    *size = n;
    return data;
}

u32 os_file_read(const char *path, void *buf, u32 max) {
    void *f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f == OS_BAD_FILE) return (u32)-1;
    unsigned long got = 0;
    ReadFile(f, buf, max, &got, 0);
    CloseHandle(f);
    return got;
}

OsFile os_file_create(const char *path) {
    return CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
}

int os_file_write(OsFile f, const void *data, u32 size) {
    unsigned long written;
    return WriteFile(f, data, size, &written, 0) && written == size ? 0 : -1;
}

void os_file_close(OsFile f) { CloseHandle(f); }

int os_file_save(const char *path, const void *data, u32 size) {
    OsFile f = os_file_create(path);
    if (f == OS_BAD_FILE) return -1;
    int r = os_file_write(f, data, size);
    os_file_close(f);
    return r;
}

/* ------------------------------------------------------------------ processus */
void os_exit(u32 code) { ExitProcess(code); }

void os_stderr_write(const void *buf, u32 size) {
    unsigned long written;
    WriteFile(GetStdHandle((unsigned long)-12), buf, size, &written, 0);
}

u32 os_stdin_read(void *buf, u32 max) {
    unsigned long got = 0;
    if (!ReadFile(GetStdHandle((unsigned long)-10), buf, max, &got, 0)) return 0;
    return got;
}

int os_get_args(const char **argv, int max) {
    static char line[32768]; /* longueur maximale d'une ligne de commande Windows : --keys des essais peut etre long */
    const char *c = GetCommandLineA();
    int n = 0, argc = 0;
    extern const unsigned nds_long_args; /* patches.json : sinon 2047 caracteres comme avant */
    int limit = nds_long_args ? (int)sizeof line - 1 : 2047;
    while (*c && n < limit) line[n++] = *c++;
    line[n] = 0;
    char *p = line;
    while (*p && argc < max) {
        while (*p == ' ') p++;
        if (!*p) break;
        char end = ' ';
        if (*p == '"') { end = '"'; p++; }
        argv[argc++] = p;
        while (*p && *p != end) p++;
        if (*p) *p++ = 0;
    }
    return argc;
}

/* ------------------------------------------------------------------ fils */
/* CreateThread ne passe qu'un pointeur : la fonction et son argument vivent ici (pas de malloc dans le runtime) */
#define MAX_STARTS 32
static struct { OsThreadFn fn; void *arg; } starts[MAX_STARTS];
static volatile long start_count;

static unsigned long WINAPI thread_entry(void *param) {
    int i = (int)(uptr)param;
    starts[i].fn(starts[i].arg);
    return 0;
}

int os_thread_start(OsThreadFn fn, void *arg, u32 stack_size) {
    long i = __sync_fetch_and_add(&start_count, 1);
    if (i >= MAX_STARTS) return 0;
    starts[i].fn = fn;
    starts[i].arg = arg;
    return CreateThread(0, stack_size, thread_entry, (void *)(uptr)i, 0, 0) != 0;
}

void os_thread_set_high_priority(void) { SetThreadPriority(GetCurrentThread(), 2); }

int os_cpu_count(void) { return (int)GetActiveProcessorCount(0xFFFF); }

void os_sleep(u32 ms) { Sleep(ms); }
void os_yield(void) { Sleep(0); }

OsEvent os_event_create(void) { return CreateEventA(0, 0, 0, 0); }
void os_event_set(OsEvent e) { SetEvent(e); }
void os_event_wait(OsEvent e) { WaitForSingleObject(e, 0xFFFFFFFFu); }

void *os_thread_self(void) {
    void *h = 0;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &h, 0, 0, 2);
    return h;
}

int os_thread_sample(void *thread, uptr *ip, uptr *sp) {
    static u64 ctx[160] __attribute__((aligned(16))); /* CONTEXT : 716 octets en x86, 1232 en x64 */
#if defined(__x86_64__)
    /* CONTEXT x64 : ContextFlags (0x30) = CONTEXT_CONTROL (0x100001) ; Rsp (0x98) et Rip (0xF8) servent */
    *(u32 *)((u8 *)ctx + 0x30) = 0x100001;
    SuspendThread(thread);
    GetThreadContext(thread, ctx);
    ResumeThread(thread);
    *ip = *(u64 *)((u8 *)ctx + 0xF8);
    *sp = *(u64 *)((u8 *)ctx + 0x98);
#else
    /* CONTEXT x86 : ContextFlags = CONTEXT_CONTROL (0x10001) ; Eip (184) et Esp (196) servent */
    u32 *c = (u32 *)ctx;
    c[0] = 0x10001;
    SuspendThread(thread);
    GetThreadContext(thread, c);
    ResumeThread(thread);
    *ip = c[184 / 4];
    *sp = c[196 / 4];
#endif
    return 1;
}

/* ------------------------------------------------------------------ temps */
s64 os_time_freq(void) {
    s64 f;
    QueryPerformanceFrequency(&f);
    return f;
}

s64 os_time_now(void) {
    s64 t;
    QueryPerformanceCounter(&t);
    return t;
}

void os_time_hires(void) { timeBeginPeriod(1); }

void os_local_time(OsTime *t) {
    struct { u16 year, month, dow, day, hour, minute, second, ms; } s; /* SYSTEMTIME */
    GetLocalTime(&s);
    t->year = s.year;
    t->month = s.month;
    t->dow = s.dow;
    t->day = s.day;
    t->hour = s.hour;
    t->minute = s.minute;
    t->second = s.second;
}

/* ------------------------------------------------------------------ memoire partagee */
#define FILE_MAP_ALL_ACCESS 0xF001F

void *os_shm_open(const char *name) {
    void *map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, name);
    return map ? MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, 0) : 0;
}

/* ------------------------------------------------------------------ exceptions */
#define EXC_ACCESS 0xC0000005u
#define EXC_STEP 0x80000004u
#define EXC_CONTINUE (-1)
#define EXC_NEXT 0

typedef struct { u32 code, flags; void *rec; void *addr; u32 nparams; uptr info[15]; } ExcRecord;
typedef struct { ExcRecord *rec; u8 *ctx; } ExcPointers;
#define CTX_U32(c, off) (*(u32 *)((c) + (off)))
#define CTX_PTR(c, off) (*(uptr *)((c) + (off)))
#if defined(__x86_64__)
/* CONTEXT x64 : EFlags (0x44), Rsp (0x98), Rbp (0xA0), Rip (0xF8) */
#define CTX_EBP 0xA0
#define CTX_EIP 0xF8
#define CTX_EFLAGS 0x44
#define CTX_ESP 0x98
#else
/* CONTEXT x86 : seuls Ebp (180), Eip (184), EFlags (192) et Esp (196) servent */
#define CTX_EBP 180
#define CTX_EIP 184
#define CTX_EFLAGS 192
#define CTX_ESP 196
#endif

static OsFaultFn fault_fn;

static long WINAPI on_exception(ExcPointers *e) {
    ExcRecord *r = e->rec;
    /* exceptions C++ (GCC, MSVC) et messages de debogage : celles des mods et de l'hote Luau, que leurs propres gestionnaires attrapent */
    if (r->code == 0x20474343u || r->code == 0xE06D7363u || r->code == 0x40010006u || r->code == 0x4001000Au || r->code == 0x406D1388u) return EXC_NEXT;
    OsFault f;
    f.kind = r->code == EXC_ACCESS ? OS_FAULT_ACCESS : r->code == EXC_STEP ? OS_FAULT_STEP : OS_FAULT_OTHER;
    f.code = r->code;
    f.ip = (uptr)r->addr;
    f.addr = r->info[1];
    f.access = r->info[0];
    f.ctx = e->ctx;
    return fault_fn(&f) == OS_FAULT_RESUME ? EXC_CONTINUE : EXC_NEXT;
}

void os_fault_install(OsFaultFn fn) {
    fault_fn = fn;
    AddVectoredExceptionHandler(1, on_exception);
}

uptr os_fault_reg(const OsFault *f, int reg) {
    return CTX_PTR((u8 *)f->ctx, reg == OS_REG_SP ? CTX_ESP : CTX_EBP);
}

void os_fault_set_step(OsFault *f, int on) {
    if (on) CTX_U32((u8 *)f->ctx, CTX_EFLAGS) |= 0x100;
    else CTX_U32((u8 *)f->ctx, CTX_EFLAGS) &= ~0x100u;
}

void os_fault_call(OsFault *f, void (*fn)(void)) {
    u8 *c = f->ctx;
    uptr sp = CTX_PTR(c, CTX_ESP) - sizeof(uptr);
    *(uptr *)sp = CTX_PTR(c, CTX_EIP);
    CTX_PTR(c, CTX_ESP) = sp;
    CTX_PTR(c, CTX_EIP) = (uptr)fn;
}

/* ------------------------------------------------------------------ son : waveOut, six tampons en anneau */
typedef struct {
    u16 tag, channels;
    u32 rate, bytes;
    u16 align, bits, extra;
} __attribute__((packed)) WaveFmt;
typedef struct {
    void *data;
    u32 len, recorded;
    uptr user;
    u32 flags, loops;
    void *next;
    uptr reserved;
} WaveHdr;

#define BUFFERS 6
#define MAX_BUF_FRAMES 1024
#define WHDR_DONE 1

struct OsAudio {
    void *dev;
    u32 frames;
    int cur;
    u32 used; /* tampons rendus au moins une fois */
    s16 buf[BUFFERS][MAX_BUF_FRAMES * 2];
    WaveHdr hdr[BUFFERS];
};
static struct OsAudio audio;

OsAudio *os_audio_open(u32 rate, u32 frames) {
    if (frames > MAX_BUF_FRAMES) return 0;
    WaveFmt fmt = {1, 2, rate, rate * 4, 4, 16, 0};
    if (waveOutOpen(&audio.dev, 0xFFFFFFFFu, &fmt, 0, 0, 0)) return 0;
    audio.frames = frames;
    for (int i = 0; i < BUFFERS; i++) {
        audio.hdr[i].data = audio.buf[i];
        audio.hdr[i].len = frames * 4;
    }
    return &audio;
}

u32 os_audio_depth(OsAudio *a) { return BUFFERS; }

s16 *os_audio_acquire(OsAudio *a) {
    if (a->used < BUFFERS) {
        waveOutPrepareHeader(a->dev, &a->hdr[a->cur], sizeof(WaveHdr));
        a->used++;
    } else {
        while (!(*(volatile u32 *)&a->hdr[a->cur].flags & WHDR_DONE)) Sleep(2);
    }
    return a->buf[a->cur];
}

void os_audio_submit(OsAudio *a) {
    waveOutWrite(a->dev, &a->hdr[a->cur], sizeof(WaveHdr));
    a->cur = (a->cur + 1) % BUFFERS;
}

/* ------------------------------------------------------------------ reseau TCP */
typedef struct { u16 family, port; u32 addr; u8 zero[8]; } SockAddr;
typedef struct { char *name; char **aliases; s16 type, length; u32 **list; } HostEnt;
IMPORT int WINAPI WSAStartup(u16, void *);
IMPORT uptr WINAPI socket(int, int, int);
IMPORT int WINAPI bind(uptr, const SockAddr *, int);
IMPORT int WINAPI listen(uptr, int);
IMPORT uptr WINAPI accept(uptr, SockAddr *, int *);
IMPORT int WINAPI connect(uptr, const SockAddr *, int);
IMPORT int WINAPI send(uptr, const void *, int, int);
IMPORT int WINAPI recv(uptr, void *, int, int);
IMPORT int WINAPI closesocket(uptr);
IMPORT int WINAPI ioctlsocket(uptr, long, u32 *);
IMPORT int WINAPI setsockopt(uptr, int, int, const void *, int);
IMPORT int WINAPI WSAGetLastError(void);
IMPORT u32 WINAPI inet_addr(const char *);
IMPORT HostEnt *WINAPI gethostbyname(const char *);
#define FIONBIO 0x8004667Eu
#define WSA_WOULD_BLOCK 10035
#define WSA_NOT_CONNECTED 10057

static void tcp_start(void) {
    static int ready;
    static u8 wsa[512];
    if (!ready) WSAStartup(0x0202, wsa);
    ready = 1;
}

static void tcp_setup(u32 s) {
    u32 on = 1;
    ioctlsocket(s, FIONBIO, &on);
    setsockopt(s, 6, 1, &on, 4); /* TCP_NODELAY */
}

u32 os_tcp_listen(u32 port) {
    tcp_start();
    u32 s = (u32)socket(2, 1, 6), on = 1;
    if (s == OS_NO_SOCKET) return s;
    setsockopt(s, 0xFFFF, 4, &on, 4); /* SO_REUSEADDR */
    SockAddr at = {2, (u16)(port << 8 | port >> 8), 0, {0}};
    if (bind(s, &at, sizeof at) || listen(s, 4)) {
        closesocket(s);
        return OS_NO_SOCKET;
    }
    ioctlsocket(s, FIONBIO, &on);
    return s;
}

u32 os_tcp_accept(u32 listener) {
    u32 s = (u32)accept(listener, 0, 0);
    if (s != OS_NO_SOCKET) tcp_setup(s);
    return s;
}

u32 os_tcp_connect(const char *host, u32 port) {
    tcp_start();
    u32 addr = inet_addr(host);
    if (addr == 0xFFFFFFFFu) {
        HostEnt *found = gethostbyname(host);
        if (!found || !found->list || !found->list[0]) return OS_NO_SOCKET;
        addr = *found->list[0];
    }
    u32 s = (u32)socket(2, 1, 6);
    if (s == OS_NO_SOCKET) return s;
    tcp_setup(s);
    SockAddr at = {2, (u16)(port << 8 | port >> 8), addr, {0}};
    connect(s, &at, sizeof at);
    return s;
}

void os_tcp_send(u32 s, const void *data, u32 size) { send(s, data, (int)size, 0); }

int os_tcp_recv(u32 s, void *buf, u32 max) {
    int got = recv(s, buf, (int)max, 0);
    if (got >= 0) return got;
    int error = WSAGetLastError();
    return error == WSA_WOULD_BLOCK || error == WSA_NOT_CONNECTED ? -1 : 0;
}

void os_tcp_close(u32 s) { closesocket(s); }
