/* Couche systeme : tout ce que le runtime demande a la plateforme (memoire, fichiers, fils, temps, son).
 * os_win32.c est la seule source qui appelle Win32 (kernel32.def, winmm.def, ws2_32.def) ; sur macOS, os_posix.c (libSystem) et
 * audio_coreaudio.c (AudioToolbox). Le reste du runtime ne voit que ceci. */
#ifndef OS_H
#define OS_H

#include "nitro.h"

/* ------------------------------------------------------------------ memoire */
enum { OS_NOACCESS = 0, OS_RW = 1 };

/* Reserve et engage `size` octets lisibles et inscriptibles, a zero. `want` = adresse exigee, ou 0 pour n'importe ou.
 * `want` peut tomber dans la fenetre de os_mem_window : la zone y est alors engagee.
 * Renvoie 0 si la plateforme refuse (l'adresse exigee n'est pas libre, memoire insuffisante). */
void *os_mem_alloc(void *want, u32 size);
#if !NDS_IDENTITY
/* Fenetre de la memoire DS (nds_mem) : reserve 4 Go + 64 Ko sans acces, a n'importe quelle adresse. 0 si impossible. */
void *os_mem_window(void);
#endif
/* Change les droits d'une zone deja allouee (OS_NOACCESS ou OS_RW), page par page. */
void os_mem_protect(void *p, u32 size, int prot);

/* ------------------------------------------------------------------ fichiers */
typedef void *OsFile;
#define OS_BAD_FILE ((OsFile)-1)

/* Lit tout le fichier dans une zone allouee par os_mem_alloc. Renvoie 0 si le fichier est introuvable. */
void *os_file_load(const char *path, u32 *size);
/* Lit au plus `max` octets du debut du fichier dans `buf`. Renvoie le nombre d'octets lus, ou (u32)-1 si le fichier ne s'ouvre pas. */
u32 os_file_read(const char *path, void *buf, u32 max);
/* Cree (ou vide) le fichier, ecrit, ferme. Renvoie 0 si tout est ecrit. */
int os_file_save(const char *path, const void *data, u32 size);
/* Fichier ecrit en plusieurs fois (sortie --wav). */
OsFile os_file_create(const char *path);
int os_file_write(OsFile f, const void *data, u32 size);
void os_file_close(OsFile f);

/* ------------------------------------------------------------------ processus */
void os_exit(u32 code);
/* Journal : ecrit sur la sortie d'erreur. */
void os_stderr_write(const void *buf, u32 size);
/* Commandes du launcher sur l'entree standard. Renvoie le nombre d'octets lus (au moins 1), ou 0 si l'entree est fermee. */
u32 os_stdin_read(void *buf, u32 max);
/* Decoupe la ligne de commande : argv[0] = le programme. Renvoie argc (au plus `max`). Les chaines restent valides. */
int os_get_args(const char **argv, int max);

/* Bibliotheque partagee (mods natifs) : 0 si elle ne se charge pas ; os_lib_sym : 0 si le nom manque. */
void *os_lib_open(const char *path);
void *os_lib_sym(void *lib, const char *name);

/* ------------------------------------------------------------------ fils */
typedef void (*OsThreadFn)(void *arg);
typedef void *OsEvent;

/* Lance un fil (la fonction ne revient normalement jamais). 0 si le fil n'a pas pu demarrer. */
int os_thread_start(OsThreadFn fn, void *arg, u32 stack_size);
void os_thread_set_high_priority(void); /* fil appelant (son) */
int os_cpu_count(void);
void os_sleep(u32 ms);
void os_yield(void);

/* Evenement a reinitialisation automatique, au repos a la creation. */
OsEvent os_event_create(void);
void os_event_set(OsEvent e);
void os_event_wait(OsEvent e); /* sans delai */

/* Bornes de la pile du fil appelant telles que le systeme les connait. Les fils du jeu tournent sur leurs propres piles : le runtime
 * les donne au systeme a chaque changement, sinon le deroulement d'une exception (C++ d'un mod, erreur d'un script Luau) les refuse.
 * Sans objet hors de Windows (rien a faire). */
void os_stack_bounds(uptr *lo, uptr *hi);
void os_set_stack_bounds(uptr lo, uptr hi);

/* Profil (--prof) : poignee du fil appelant, utilisable depuis un autre fil, puis echantillon du fil suspendu. */
void *os_thread_self(void);
int os_thread_sample(void *thread, uptr *ip, uptr *sp);

/* ------------------------------------------------------------------ temps */
s64 os_time_freq(void);  /* ticks par seconde */
s64 os_time_now(void);
void os_time_hires(void); /* demande des attentes fines (Sleep(1) vraiment court) */

typedef struct { u32 year, month, dow, day, hour, minute, second; } OsTime; /* year sur 4 chiffres, month 1-12, dow 0 = dimanche */
void os_local_time(OsTime *t);

/* ------------------------------------------------------------------ memoire partagee avec le launcher */
/* Ouvre la memoire d'images creee par le launcher sous ce nom et la projette. 0 si introuvable. */
void *os_shm_open(const char *name);

/* ------------------------------------------------------------------ reseau TCP sans attente (net.c) */
#define OS_NO_SOCKET 0xFFFFFFFFu
/* Ecoute sur ce port. OS_NO_SOCKET si le port ne s'ouvre pas. */
u32 os_tcp_listen(u32 port);
/* OS_NO_SOCKET tant que personne n'arrive. */
u32 os_tcp_accept(u32 listener);
/* Commence la connexion, sans l'attendre. Le nom de l'hote, lui, est resolu tout de suite. OS_NO_SOCKET si c'est impossible. */
u32 os_tcp_connect(const char *host, u32 port);
void os_tcp_send(u32 s, const void *data, u32 size);
/* Octets lus, 0 si la connexion est perdue, -1 s'il n'y a rien pour l'instant (ou si elle n'est pas encore etablie). */
int os_tcp_recv(u32 s, void *buf, u32 max);
void os_tcp_close(u32 s);

/* ------------------------------------------------------------------ exceptions du processeur (registres a effets, --with-c) */
enum { OS_FAULT_OTHER = 0, OS_FAULT_ACCESS, OS_FAULT_STEP };
enum { OS_REG_SP = 0, OS_REG_BP };
enum { OS_FAULT_PASS = 0, OS_FAULT_RESUME };

typedef struct {
    int kind;
    u32 code;   /* code natif, pour les journaux */
    uptr ip;    /* adresse de l'instruction fautive */
    uptr addr;  /* ACCESS : adresse touchee (de l'hote : HOST2DS pour l'adresse DS) */
    u32 access; /* ACCESS : 0 lecture, 1 ecriture, autre = execution */
    void *ctx;  /* registres du fil, pour os_fault_* */
} OsFault;

typedef int (*OsFaultFn)(OsFault *f); /* OS_FAULT_RESUME : reprendre ; OS_FAULT_PASS : laisser au suivant */
void os_fault_install(OsFaultFn fn);
uptr os_fault_reg(const OsFault *f, int reg);
void os_fault_set_step(OsFault *f, int on);      /* execution pas a pas : une exception STEP apres la prochaine instruction */
void os_fault_call(OsFault *f, void (*fn)(void)); /* a la reprise, fn s'execute puis rend la main a l'instruction fautive */

/* ------------------------------------------------------------------ son */
typedef struct OsAudio OsAudio;
/* Sortie stereo 16 bits ; `frames` echantillons par tampon. Renvoie 0 s'il n'y a pas de sortie son. */
OsAudio *os_audio_open(u32 rate, u32 frames);
/* Nombre de tampons en file : autant d'acquire/submit pour amorcer la sortie avant qu'elle ne consomme. */
u32 os_audio_depth(OsAudio *a);
/* Rend le prochain tampon libre (attend qu'il le soit) ; le remplir puis le rendre avec os_audio_submit. */
s16 *os_audio_acquire(OsAudio *a);
void os_audio_submit(OsAudio *a);

#endif
