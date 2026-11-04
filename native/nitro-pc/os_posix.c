/* Couche systeme pour macOS 64 bits (arm64, x86_64) : tous les appels systeme du runtime sont ici, avec libSystem.
 * Le son est dans audio_coreaudio.c. Voir native/MACOS.md 3.4.
 * Les en-tetes systeme passent avant nitro.h, dont les macros (IO, DS, REG32...) ne doivent pas les toucher. */
#if !defined(__APPLE__)
#error "os_posix.c : macOS seulement pour l'instant (Linux : arguments par /proc/self/cmdline, sem_t au lieu de dispatch)"
#endif

#include <crt_externs.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "nitro.h"
#include "os.h"

_Static_assert(sizeof(void *) == 8, "os_posix.c : 64 bits seulement");

/* ------------------------------------------------------------------ point d'entree */
/* Windows entre directement dans nds_start (ld -e) ; ici dyld appelle main. Le test du son a son propre main. */
#ifndef OS_NO_MAIN
void nds_start(void);
int main(void) {
    nds_start();
    return 3;
}
#endif

/* ------------------------------------------------------------------ memoire */
/* Fenetre DS : 4 Go + 64 Ko de garde (un acces de 4 octets a 0xFFFFFFFF deborde) reserves sans acces.
 * os_mem_alloc engage ensuite dedans les zones de nds_start et les piles de stack_alloc. */
#define WINDOW_SIZE (0x100000000ull + 0x10000u)
static u8 *window;

static uptr page_size(void) {
    static uptr ps;
    if (!ps) ps = (uptr)sysconf(_SC_PAGESIZE); /* 16 Ko sur Apple Silicon, 4 Ko sur Intel */
    return ps;
}

void *os_mem_window(void) {
    if (window) return window;
    void *p = mmap(0, WINDOW_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) return 0;
    window = p;
    return p;
}

static int in_window(const void *p, uptr size) {
    uptr a = (uptr)p, w = (uptr)window;
    return window && a >= w && a - w <= WINDOW_SIZE && size <= WINDOW_SIZE - (a - w);
}

void *os_mem_alloc(void *want, u32 size) {
    if (!size) size = 1;
    if (want && in_window(want, size)) {
        /* comme MEM_COMMIT : les pages deja engagees gardent leur contenu, les nouvelles sont a zero.
         * mprotect veut une adresse alignee sur la page (16 Ko) : les zones de nds_start le sont, sauf les petites
         * (palettes, OAM : 4 Ko), dont la page entiere devient accessible. */
        uptr ps = page_size(), a = (uptr)want & ~(ps - 1), end = ((uptr)want + size + ps - 1) & ~(ps - 1);
        if (mprotect((void *)a, end - a, PROT_READ | PROT_WRITE)) return 0;
        return want;
    }
    void *p = mmap(want, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED) return 0;
    if (want && p != want) { /* VirtualAlloc echoue si l'adresse exigee n'est pas libre */
        munmap(p, size);
        return 0;
    }
    return p;
}

void os_mem_protect(void *p, u32 size, int prot) {
    uptr ps = page_size(), a = (uptr)p & ~(ps - 1), end = ((uptr)p + size + ps - 1) & ~(ps - 1);
    mprotect((void *)a, end - a, prot == OS_RW ? PROT_READ | PROT_WRITE : PROT_NONE);
}

/* ------------------------------------------------------------------ fichiers */
/* OsFile porte le descripteur ; -1 donne OS_BAD_FILE */
#define FD(f) ((int)(__INTPTR_TYPE__)(f))
#define FILE_OF(fd) ((OsFile)(__INTPTR_TYPE__)(fd))

static int open_retry(const char *path, int flags, int mode) {
    int fd;
    do fd = open(path, flags | O_CLOEXEC, mode);
    while (fd < 0 && errno == EINTR);
    return fd;
}

/* lit jusqu'a `max` octets ou la fin du fichier */
static u32 read_all(int fd, void *buf, u32 max) {
    u32 got = 0;
    while (got < max) {
        ssize_t n = read(fd, (u8 *)buf + got, max - got);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        got += (u32)n;
    }
    return got;
}

static int write_all(int fd, const void *data, u32 size) {
    u32 done = 0;
    while (done < size) {
        ssize_t n = write(fd, (const u8 *)data + done, size - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        done += (u32)n;
    }
    return 0;
}

void os_stack_bounds(uptr *lo, uptr *hi) { *lo = *hi = 0; }
void os_set_stack_bounds(uptr lo, uptr hi) {}

void *os_lib_open(const char *path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
void *os_lib_sym(void *lib, const char *name) { return dlsym(lib, name); }

void *os_file_load(const char *path, u32 *size) {
    int fd = open_retry(path, O_RDONLY, 0);
    if (fd < 0) return 0;
    struct stat st;
    if (fstat(fd, &st) || st.st_size > 0xFFFFFFFFll) { close(fd); return 0; }
    u32 n = (u32)st.st_size;
    u8 *data = os_mem_alloc(0, n);
    if (!data) { close(fd); return 0; }
    read_all(fd, data, n);
    close(fd);
    *size = n;
    return data;
}

u32 os_file_read(const char *path, void *buf, u32 max) {
    int fd = open_retry(path, O_RDONLY, 0);
    if (fd < 0) return (u32)-1;
    u32 got = read_all(fd, buf, max);
    close(fd);
    return got;
}

OsFile os_file_create(const char *path) {
    return FILE_OF(open_retry(path, O_WRONLY | O_CREAT | O_TRUNC, 0644));
}

int os_file_write(OsFile f, const void *data, u32 size) { return write_all(FD(f), data, size); }

void os_file_close(OsFile f) { close(FD(f)); }

int os_file_save(const char *path, const void *data, u32 size) {
    OsFile f = os_file_create(path);
    if (f == OS_BAD_FILE) return -1;
    int r = os_file_write(f, data, size);
    os_file_close(f);
    return r;
}

/* ------------------------------------------------------------------ processus */
/* _exit et pas exit : les autres fils (rendu, son) tournent encore, aucun destructeur ne doit passer sous eux */
void os_exit(u32 code) { _exit((int)code); }

void os_stderr_write(const void *buf, u32 size) { write_all(2, buf, size); }

u32 os_stdin_read(void *buf, u32 max) {
    for (;;) {
        ssize_t n = read(0, buf, max);
        if (n < 0 && errno == EINTR) continue;
        return n > 0 ? (u32)n : 0;
    }
}

int os_get_args(const char **argv, int max) {
    int argc = *_NSGetArgc();
    char **v = *_NSGetArgv();
    if (argc > max) argc = max;
    for (int i = 0; i < argc; i++) argv[i] = v[i];
    return argc;
}

/* ------------------------------------------------------------------ fils */
typedef struct { OsThreadFn fn; void *arg; } Start;

static void *thread_entry(void *param) {
    Start s = *(Start *)param;
    free(param);
    s.fn(s.arg);
    return 0;
}

int os_thread_start(OsThreadFn fn, void *arg, u32 stack_size) {
    Start *s = malloc(sizeof *s);
    if (!s) return 0;
    s->fn = fn;
    s->arg = arg;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (stack_size) {
        uptr ps = page_size(), sz = ((uptr)stack_size + ps - 1) & ~(ps - 1);
        if (sz < PTHREAD_STACK_MIN) sz = PTHREAD_STACK_MIN;
        pthread_attr_setstacksize(&attr, sz);
    }
    pthread_t t;
    int r = pthread_create(&t, &attr, thread_entry, s);
    pthread_attr_destroy(&attr);
    if (r) { free(s); return 0; }
    return 1;
}

/* fil du son : la classe de service la plus haute, que le systeme ne fait pas attendre derriere le rendu */
void os_thread_set_high_priority(void) { pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0); }

int os_cpu_count(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
}

void os_sleep(u32 ms) {
    struct timespec t = {ms / 1000, (long)(ms % 1000) * 1000000}, rest;
    while (nanosleep(&t, &rest) && errno == EINTR) t = rest;
}

void os_yield(void) { sched_yield(); }

/* Evenement a reinitialisation automatique (CreateEventA(0, 0, 0, 0)) : `set` est un drapeau, le semaphore ne sert
 * qu'a reveiller. set ne signale que si le drapeau passe de 0 a 1 ; wait consomme le drapeau, sinon dort.
 * Plusieurs set avant un wait n'en donnent qu'un, comme sous Windows ; un jeton reste parfois en trop dans le semaphore :
 * le wait suivant fait alors un tour de plus et se rendort, rien n'est perdu. */
typedef struct {
    dispatch_semaphore_t sem;
    int flag;
} Event;

OsEvent os_event_create(void) {
    Event *e = malloc(sizeof *e);
    if (!e) return 0;
    e->sem = dispatch_semaphore_create(0);
    e->flag = 0;
    return e;
}

void os_event_set(OsEvent ev) {
    Event *e = ev;
    if (!__atomic_exchange_n(&e->flag, 1, __ATOMIC_SEQ_CST)) dispatch_semaphore_signal(e->sem);
}

void os_event_wait(OsEvent ev) {
    Event *e = ev;
    while (!__atomic_exchange_n(&e->flag, 0, __ATOMIC_SEQ_CST)) dispatch_semaphore_wait(e->sem, DISPATCH_TIME_FOREVER);
}

/* --prof : port Mach du fil, suspendu le temps de lire pc et sp */
void *os_thread_self(void) { return (void *)(uptr)pthread_mach_thread_np(pthread_self()); }

int os_thread_sample(void *thread, uptr *ip, uptr *sp) {
    thread_act_t t = (thread_act_t)(uptr)thread;
    *ip = *sp = 0;
    if (thread_suspend(t) != KERN_SUCCESS) return 0;
#if defined(__arm64__)
    arm_thread_state64_t s;
    mach_msg_type_number_t n = ARM_THREAD_STATE64_COUNT;
    kern_return_t r = thread_get_state(t, ARM_THREAD_STATE64, (thread_state_t)&s, &n);
    if (r == KERN_SUCCESS) {
        *ip = (uptr)arm_thread_state64_get_pc(s);
        *sp = (uptr)arm_thread_state64_get_sp(s);
    }
#elif defined(__x86_64__)
    x86_thread_state64_t s;
    mach_msg_type_number_t n = x86_THREAD_STATE64_COUNT;
    kern_return_t r = thread_get_state(t, x86_THREAD_STATE64, (thread_state_t)&s, &n);
    if (r == KERN_SUCCESS) {
        *ip = (uptr)s.__rip;
        *sp = (uptr)s.__rsp;
    }
#else
#error "os_thread_sample : architecture inconnue"
#endif
    thread_resume(t);
    return r == KERN_SUCCESS;
}

/* ------------------------------------------------------------------ temps */
/* mach_absolute_time : 24 MHz sur Apple Silicon (base 125/3), la nanoseconde sur Intel (1/1) */
s64 os_time_freq(void) {
    mach_timebase_info_data_t tb;
    mach_timebase_info(&tb);
    return (s64)(1000000000ull * tb.denom / tb.numer);
}

s64 os_time_now(void) { return (s64)mach_absolute_time(); }

void os_time_hires(void) {} /* nanosleep est deja fin */

void os_local_time(OsTime *t) {
    time_t now = time(0);
    struct tm tm;
    localtime_r(&now, &tm);
    t->year = (u32)tm.tm_year + 1900;
    t->month = (u32)tm.tm_mon + 1;
    t->dow = (u32)tm.tm_wday;
    t->day = (u32)tm.tm_mday;
    t->hour = (u32)tm.tm_hour;
    t->minute = (u32)tm.tm_min;
    t->second = (u32)tm.tm_sec;
}

/* ------------------------------------------------------------------ memoire partagee */
/* Le launcher cree l'objet (shm_open + ftruncate, nom "/ndsgs-<pid>-<code>", 31 caracteres au plus) et le detruit. */
void *os_shm_open(const char *name) {
    char buf[64];
    if (name[0] != '/') { /* nom a la Windows : POSIX veut un / au debut */
        u32 n = 0;
        buf[n++] = '/';
        while (*name && n < sizeof buf - 1) buf[n++] = *name++;
        buf[n] = 0;
        name = buf;
    }
    int fd = shm_open(name, O_RDWR, 0);
    if (fd < 0) return 0;
    struct stat st;
    void *p = MAP_FAILED;
    if (!fstat(fd, &st) && st.st_size > 0) p = mmap(0, (size_t)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return p == MAP_FAILED ? 0 : p;
}

/* ------------------------------------------------------------------ exceptions */
/* SIGSEGV et SIGBUS (une page PROT_NONE donne SIGBUS sur macOS). En 64 bits il n'y a que le mode traduit
 * (MACOS.md 3.5) : l'exception sert au journal d'un acces invalide, jamais au piege des registres ni au pas a pas. */
static OsFaultFn fault_fn;

static void on_signal(int sig, siginfo_t *si, void *uc_) {
    ucontext_t *uc = uc_;
    OsFault f;
    f.kind = OS_FAULT_ACCESS;
    f.code = (u32)sig;
    f.addr = (uptr)si->si_addr;
    f.ctx = uc;
#if defined(__arm64__)
    f.ip = (uptr)arm_thread_state64_get_pc(uc->uc_mcontext->__ss);
    f.access = (uc->uc_mcontext->__es.__esr >> 6) & 1; /* ESR_ELx.WnR */
#elif defined(__x86_64__)
    f.ip = (uptr)uc->uc_mcontext->__ss.__rip;
    f.access = (uc->uc_mcontext->__es.__err >> 1) & 1; /* code d'erreur de #PF : bit W */
#endif
    if (fault_fn && fault_fn(&f) == OS_FAULT_RESUME) return;
    /* laisse la main au systeme : l'instruction refait la faute, avec l'action par defaut (rapport de plantage) */
    signal(sig, SIG_DFL);
}

void os_fault_install(OsFaultFn fn) {
    fault_fn = fn;
    /* pile de secours : un debordement de pile doit encore pouvoir ecrire son journal */
    static u8 alt[64 * 1024];
    stack_t ss = {.ss_sp = alt, .ss_size = sizeof alt, .ss_flags = 0};
    sigaltstack(&ss, 0);
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
}

uptr os_fault_reg(const OsFault *f, int reg) {
    const ucontext_t *uc = f->ctx;
#if defined(__arm64__)
    return reg == OS_REG_SP ? (uptr)arm_thread_state64_get_sp(uc->uc_mcontext->__ss)
                            : (uptr)arm_thread_state64_get_fp(uc->uc_mcontext->__ss);
#elif defined(__x86_64__)
    return reg == OS_REG_SP ? (uptr)uc->uc_mcontext->__ss.__rsp : (uptr)uc->uc_mcontext->__ss.__rbp;
#endif
}

/* piege des registres (--with-c) : 32 bits seulement */
static void no_trap(void) {
    static const char msg[] = "piege des registres sans version 64 bits (--with-c)\n";
    os_stderr_write(msg, sizeof msg - 1);
    _exit(5);
}
void os_fault_set_step(OsFault *f, int on) { if (on) no_trap(); }
void os_fault_call(OsFault *f, void (*fn)(void)) { no_trap(); }

/* ------------------------------------------------------------------ reseau TCP */
static void tcp_setup(int s) {
    int on = 1;
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof on);
    setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof on); /* sans cela, ecrire a un joueur parti arrete le jeu */
}

u32 os_tcp_listen(u32 port) {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), on = 1;
    if (s < 0) return OS_NO_SOCKET;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    struct sockaddr_in at = {.sin_family = AF_INET, .sin_port = htons((u16)port), .sin_addr.s_addr = htonl(INADDR_ANY)};
    if (bind(s, (struct sockaddr *)&at, sizeof at) || listen(s, 4)) {
        close(s);
        return OS_NO_SOCKET;
    }
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    return (u32)s;
}

u32 os_tcp_accept(u32 listener) {
    int s = accept((int)listener, 0, 0);
    if (s < 0) return OS_NO_SOCKET;
    tcp_setup(s);
    return (u32)s;
}

u32 os_tcp_connect(const char *host, u32 port) {
    struct addrinfo want = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM}, *found = 0;
    if (getaddrinfo(host, 0, &want, &found) || !found) return OS_NO_SOCKET;
    struct sockaddr_in at = *(struct sockaddr_in *)found->ai_addr;
    freeaddrinfo(found);
    at.sin_port = htons((u16)port);
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) return OS_NO_SOCKET;
    tcp_setup(s);
    connect(s, (struct sockaddr *)&at, sizeof at);
    return (u32)s;
}

void os_tcp_send(u32 s, const void *data, u32 size) { (void)!send((int)s, data, size, 0); }

int os_tcp_recv(u32 s, void *buf, u32 max) {
    ssize_t got = recv((int)s, buf, max, 0);
    if (got >= 0) return (int)got;
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOTCONN || errno == EINPROGRESS || errno == EINTR ? -1 : 0;
}

void os_tcp_close(u32 s) { close((int)s); }
