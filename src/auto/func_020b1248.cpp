#include "m2c.h"

// Partial interface: unknown virtuals keep the observed slot positions.
// Declarations only: the vtable stays in the ROM data.
class HeroObj {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void action(int, int, int, int); // Slot 0x24.
};

typedef s32 BOOL;

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    u16 v;
} Ang;

typedef struct {
    Ang x, y, z, w;
} Rot;

typedef struct {
    s16 x, y, z;
} Rot16;

typedef struct {
    s32 mtx[9];
    Vec3 pos;
    Vec3 scale;
    Rot16 rot;
    u8 pad42[2];
    u8 f44;
    u8 f45;
} Xform;

typedef struct {
    u8 len;
    u8 type;
    u16 value;
    union {
        u8 b[0x24];
        s8 sb[0x24];
        u16 h[0x12];
        s32 w[9];
        Vec3 v;
        struct {
            s32 id;
            Vec3 v;
        } idv;
        struct {
            u16 h[3][5];
            u8 b[3];
        } slots;
        struct {
            u8 pad[6];
            Ang a;
        } ang;
    } u;
} Record;

typedef struct Sub {
    u8 pad0[0xe];
    u8 fe;
} Sub;

struct Ent;

typedef struct EntVt {
    void *pad0[16];
    void (*f40)(struct Ent *, s32);
} EntVt;

typedef struct Ent {
    EntVt *vt;
    u8 pad4[4];
    u32 flags;
    u16 mode;
    u8 padE[0x40 - 0xe];
    u8 f40;
    u8 pad41[0xb0 - 0x41];
    Vec3 pos;
    u8 padBC[0xc8 - 0xbc];
    Rot rot;
    u8 rot_dirty;
    u8 pos_dirty;
    u8 padD2[0x5e0 - 0xd2];
    u8 list[0x18];
    Sub *f5f8;
    u8 pad5fc[0x6a0 - 0x5fc];
    Vec3 f6a0;
} Ent;

struct Hero;

typedef struct HeroVt {
    void *pad0[9];
    void (*f24)(struct Hero *, s32, s32, s32, s32);
} HeroVt;

typedef struct Hero {
    HeroVt *vt;
    u8 pad4[0x60 - 4];
    Vec3 f60;
    u8 pad6c[0x6bb - 0x6c];
    u8 f6bb;
} Hero;

typedef struct {
    u8 pad0[0x28];
    u8 part[4];
} Cam;

typedef struct {
    u8 pad0[4];
    Cam *cam;
} Scene;

typedef struct {
    u8 pad0[0x38c];
    Scene *scene;
} World;

typedef struct {
    u8 pad0[0x4c];
    Hero *hero[2];
    u8 pad54[0x68 - 0x54];
    World *world;
} Globals;

typedef struct {
    u8 pad0[4];
    s32 kind;
} Node;

typedef struct {
    Node *f0;
    Node *f4;
    u8 pad8[0x28 - 8];
    s32 f28;
    u8 pad2c[0x58 - 0x2c];
    s32 f58;
} Pres;

struct VObj;

typedef struct VObjVt {
    void *pad0[6];
    void (*f18)(struct VObj *, s32);
    s32 (*f1c)(struct VObj *);
} VObjVt;

typedef struct VObj {
    VObjVt *vt;
} VObj;

typedef struct {
    u8 pad0[6];
    u8 f6;
    u8 pad7;
    s32 f8;
    u8 fc;
    u8 padD[3];
    void *f10;
    u8 pad14[0x3c - 0x14];
    u32 f3c;
    u8 f40;
    s8 f41;
    u8 f42;
    u8 pad43;
    Vec3 pos;
    u8 f50;
} Sess;

typedef struct {
    u8 pad0[0x29];
    u8 count;
    u8 local_id;
} Cfg;

typedef struct {
    u16 level;
    u16 bars[5];
    u8 padC[0x1a - 0xc];
    u8 f1a;
    u8 f1b;
    u8 pad1c[0x40 - 0x1c];
} Hud;

typedef struct {
    u16 h[5];
    u8 b;
    u8 pad;
} Slot;

typedef struct {
    u8 pad0[0x41];
    s8 f41;
} Player;

typedef struct {
    u8 pad0[6];
    u8 f6;
} Menu;

typedef struct {
    u32 flag0 : 1;
    u32 rest : 31;
    u8 pad4[0x50 - 4];
    u8 f50;
    s8 f51;
    u8 pad52[0x2248 - 0x52];
    Hud hud[4];
    u8 pad2348[0x28d1 - 0x2348];
    u8 f28d1;
    u8 pad28d2[0x2ad0 - 0x28d2];
    Player *player;
    u8 pad2ad4[0x2bb8 - 0x2ad4];
    Menu *menu;
    u8 pad2bbc[0x443e - 0x2bbc];
    Slot slots[4][3];
} Data;

typedef struct {
    u16 kind;
    u8 pad2[0xe];
    u8 f10;
    u8 pad11[7];
} Params;

typedef struct {
    u32 id;
    u16 name[10];
    u8 pad18[0x5c - 0x18];
} NameMsg;

typedef struct {
    u8 month;
    u8 day;
} OSBirthday;

typedef struct {
    u8 language;
    u8 favoriteColor;
    OSBirthday birthday;
    u16 nickName[10];
    u16 nickNameLength;
    u16 comment[26];
    u16 commentLength;
} OSOwnerInfo;

extern "C" {
extern VObj *data_021092f4;
extern Pres data_02102900;
extern u8 data_0210794c[];
extern Globals data_0210930c[];
extern Ent *data_02109358[];
extern Cfg data_021027f8;
extern Sess *data_02107860;
extern Data data_02104d90;
extern u8 data_02104df4[];
extern s32 data_02101ad4;
extern Vec3 data_020ff9b4;
extern const u16 data_020f79c0[];
extern u8 data_02107654[];
extern s8 data_021090d0[];

void func_02001688(s32 arg0);
void func_020016c0(void);
Ent *func_020029c4(void *, s32, s32, s32, Xform *);
void func_02004b78(void *, s32);
void func_02006220(void *, void *);
Ent *func_0200fdd8(u32 key);
void *func_02015138(u32 kind, u32 flags, void *transform);
void func_02015dcc(void *self, u8 a1, u8 a2, u8 a3, u8 a4, u8 a5);
void func_0202c1a8(void *, void *);
void *func_020363f4(void *, Ent *, s8);
void func_020370ec(void *);
void *func_02037128(void *, Ent *, void *);
void func_02048a54(void *, u8);
void func_0205ce5c(void *, u8);
void *func_0205e80c(s32, s32, s32);
void func_0205ecc4(void *, s32, s32, s32, s32, s32);
void func_02060158(void *, s32, s32, s32, ...);
s32 func_02060198(void *);
void func_020601ac(void *, s32, s32, s32, s32, s32);
void func_02062a40(s32);
s16 **func_02066ff0(void *, u32, u32);
void func_020675d8(void *, s32);
void func_0206763c(void *, s32);
void func_0206769c(void *, s32, s32);
void func_0206dd60(void);
void func_02072bc8(void);
void func_02072cf4(s32);
void func_02073054(s32);
void func_020730d0(s32);
void func_02073604(void *);
void func_0207375c(void *);
void func_0207376c(void *, u8, void *);
void func_0207b370(void *, u8, u8);
void func_0207b3bc(void *, u8);
void func_0207d8f8(void *);
void func_0207da48(void *);
void func_020861bc(void *, s32, s32);
void func_0208642c(void *);
void func_02086458(void *, s32);
s32 func_02086534(void *, s32);
void func_02086810(void *, s32, s32);
void func_020871d8(void *, s8);
void func_0208896c(void *);
void func_02088b78(void *);
void func_02088b84(void *, u8);
void func_0208ce8c(void *);
void func_0208fa1c(void *, s32, u8, u8);
void func_0208fec8(void *, void *);
void func_02090124(void *, s32, void *);
Ent *func_02090668(void *, s32, s32);
s32 func_020917b4(void *);
void *func_020917c8(void *, s32, s32);
void func_020922f4(void *, s32, s32);
Ent *func_020931b0(void *, s32);
void func_02093690(void *, void *);
void func_02093c6c(void *, s32);
u8 func_02093fd0(s32);
void func_0209678c(void *, void *);
void *func_02096adc(void *, s32);
void func_02097cb8(u32 a, u32 b, u32 c, u32 d, u32 e);
void func_02099334(void *);
void func_02099ba0(void *, u8, void *);
void func_0209b6d4(void *, s32);
s32 func_0209b6e4(void *);
void func_0209bbf0(void *);
void func_0209bce0(void *, s32, s32);
void func_0209bcf8(void *, s32, void *);
void func_0209bd50(void *, s32);
void func_0209d038(void *, s32);
void func_0209d150(void *, u8);
void func_0209e6f4(void *, u8, void *);
void func_020a3458(void *, s8);
void func_020a9a24(void *, s32, s32);
void func_020aa890(void *, void *);
u8 func_020aae08(void *, u8);
void *func_020aaeb4(void);
void func_020accac(Xform *, const Vec3 *, Rot *, Vec3 *);
void func_020b04f0(void *obj, u8 id, void *src, u8 size);
void func_020b111c(void *, s32);
void func_020b2f80(void *, Record *);
void OS_GetOwnerInfo(OSOwnerInfo *info);
}

static inline s32 node_kind(Node *n) {
    return n != NULL ? n->kind : 0;
}

static inline void node_cmd(Node *node, u8 type, void *arg) {
    if (node_kind(node) == 0x28) {
        switch (type) {
            case 2:
            case 3:
            case 6:
                func_0207376c(node, type, arg);
                break;
            case 4:
                func_0207375c(node);
                break;
            case 9:
                break;
        }
    } else if (node_kind(node) == 0x29) {
        switch (type) {
            case 2:
            case 3:
                func_02099ba0(node, type, arg);
                break;
            case 4:
                func_02099334(node);
                break;
            case 9:
                func_02099ba0(node, type, arg);
                break;
        }
    } else if (node_kind(node) == 0x36) {
        switch (type) {
            case 3:
                func_0209e6f4(node, type, arg);
                break;
        }
    }
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline u32 ent_flags(const Ent *e) {
    return e->flags;
}

static inline void vec_make(Vec3 *d, s32 x, s32 y, s32 z) {
    d->x = x;
    d->y = y;
    d->z = z;
}
static inline void xform_at(Xform *xf, s16 **q, Vec3 *scale, Rot *rot) {
    Vec3 pos;
    const s16 *d;
    scale->x = 0x1000;
    scale->y = 0x1000;
    scale->z = 0x1000;
    rot->x.v = 0;
    rot->y.v = 0;
    rot->z.v = 0;
    d        = *q;
    vec_make(&pos, d[0] << 12, d[1] << 12, d[2] << 12);
    func_020accac(xf, &pos, rot, scale);
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline const Vec3 *ent_pos(const Ent *e) {
    return &e->pos;
}

static inline void ent_set_pos(Ent *e, const Vec3 *p) {
    if (vec_ne(&e->pos, p)) {
        vec_set(&e->pos, p);
        e->pos_dirty = 1;
    }
}

// ang_eq takes b by value and compares through &b, so b keeps its own stack slot: the target
// stores the new angle twice (case 16: sp+0x3e and sp+0x22) and never reloads it.
static inline bool ang_eq_p(const Ang *a, const Ang *b) {
    return a->v == b->v;
}

static inline bool ang_eq(const Ang *a, Ang b) {
    return ang_eq_p(a, &b);
}

static inline void ang_assign(Ang *d, Ang s) {
    d->v = s.v;
}

static inline void ent_set_rot(Ent *e, const Rot *r) {
    int same = ang_eq(&r->x, e->rot.x) && ang_eq(&r->y, e->rot.y) && ang_eq(&r->z, e->rot.z);
    if (!same) {
        ang_assign(&e->rot.x, r->x);
        ang_assign(&e->rot.y, r->y);
        ang_assign(&e->rot.z, r->z);
        e->rot_dirty = 1;
    }
}

static inline void ent_set_roty(Ent *e, Ang a) {
    if (!ang_eq(&e->rot.y, a)) {
        ang_assign(&e->rot.y, a);
        e->rot_dirty = 1;
    }
}

// v * 8/pi, rounded: 0x28BE60DB9391 = round(2^47 / pi), i.e. 8/pi scaled by 2^44, and
// 8/pi = 0x10000 / (2 * pi * 0x1000): 0x1000-per-radian units to a 0x10000-per-turn angle.
static inline u16 fx_to_ang(s32 v) {
    return (u16) (((s64) v * 0x28BE60DB9391LL + 0x80000000000LL) >> 44);
}

static inline void ent_place(Ent *e, s16 **q, Vec3 *t, Rot *r) {
    int same;
    vec_make(t, (*q)[0] << 12, (*q)[1] << 12, (*q)[2] << 12);
    ent_set_pos(e, t);
    r->x.v = 0;
    r->y.v = fx_to_ang((*q)[3]);
    r->z.v = 0;
    same   = ang_eq(&r->x, e->rot.x) && ang_eq(&r->y, e->rot.y) && ang_eq(&r->z, e->rot.z);
    if (!same) {
        ang_assign(&e->rot.x, r->x);
        ang_assign(&e->rot.y, r->y);
        ang_assign(&e->rot.z, r->z);
        e->rot_dirty = 1;
    }
}

static inline void xform_init(Xform *t) {
    t->pos.x   = 0;
    t->pos.y   = 0;
    t->pos.z   = 0;
    t->scale.x = 0x1000;
    t->scale.y = 0x1000;
    t->scale.z = 0x1000;
    t->rot.x   = 0;
    t->rot.y   = 0;
    t->rot.z   = 0;
    t->f44     = 1;
    t->f45     = 1;
}

static inline void xform_make(Xform *xf, const Vec3 *pos, Vec3 *scale, Rot *rot) {
    scale->x = 0x1000;
    scale->y = 0x1000;
    scale->z = 0x1000;
    rot->x.v = 0;
    rot->y.v = 0;
    rot->z.v = 0;
    func_020accac(xf, pos, rot, scale);
}

static inline Sess **sess_ref(void) {
    return &data_02107860;
}

static inline BOOL is_remote(u8 idx) {
    return idx != 0 && idx != data_021027f8.local_id;
}

extern "C" void func_020b1248(void *mgr, Record *rec, u8 idx) {
    u8 type;
    Node *node;
    Ent *e;
    Ent *e2;
    Ent *o;
    s16 **q;
    s16 *d;
    void *p;
    Sess *m;
    s8 slot;
    u8 i;
    u8 j;
    u32 k;
    u32 n;
    Params params;
    Vec3 v28;
    Vec3 v29;
    Vec3 v30;
    Vec3 v35;
    Vec3 v56;
    Vec3 v58;
    Vec3 t15;
    Vec3 t30;
    Vec3 scale;
    Rot r15;
    Rot r30;
    Rot rot;

    type = rec->type;
    switch (type) {
        case 5:
            if (data_021092f4->vt->f1c(data_021092f4) == 0 && idx == 1) {
                if (node_kind(data_02102900.f4) == 0x28) {
                    func_02073604(data_02102900.f4);
                    func_020b04f0(data_0210794c, 5, 0, 0xff);
                }
            } else if (idx == 0) {
                func_02073604(data_02102900.f4);
            }
            break;
        case 2:
        case 3:
        case 4:
        case 6:
        case 9:
            node_cmd(data_02102900.f4, type, &rec->u);
            break;
        case 7:
            func_020917c8(data_0210930c, rec->u.b[2], rec->u.h[0]);
            e = data_02109358[rec->u.b[2]];
            if (e->f40 == 0) {
                func_02004b78(e, 0xd);
            }
            break;
        case 8:
            func_0205ecc4(&data_02102900, 4, 0, 9, rec->u.b[0], rec->u.b[1]);
            func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
            break;
        case 10:
            func_02062a40(rec->u.h[2]);
            func_02072cf4(rec->u.b[6]);
            break;
        case 11:
            func_02072bc8();
            func_0205ecc4(&data_02102900, 4, 0, 0x11, 0xe, 0);
            func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
            func_0205ce5c(&data_021027f8, 1);
            break;
        case 12:
            func_02072bc8();
            func_0205ecc4(&data_02102900, 4, 0, 0x11, 9, 0);
            func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
            func_0205ce5c(&data_021027f8, 1);
            break;
        case 13: {
            Sess **ms = sess_ref();
        // FAKE: always-true null test keeps ms out of copy propagation (address held in a register)
            if (ms) {
                func_0205ecc4(&data_02102900, 4, 0, 0xf, (*ms)->f40, 0);
                func_0205ecc4(&data_02102900, 0x2a, 1, 0, 0, 0);
                func_0209b6e4(*ms);
            }
            break;
        }
        case 14:
            func_020b2f80(mgr, rec);
            break;
        case 15: {
            func_020a3458(func_020aaeb4(), 1);
            e = data_02109358[rec->u.b[1]];
            q = func_02066ff0(data_0210930c->world->scene, rec->u.b[0], 0);
            ent_place(e, q, &t15, &r15);
            if (rec->u.b[1] != data_02104d90.player->f41) {
                break;
            }
            e2 = func_020931b0(data_0210930c, 0x17);
            ent_set_pos(e2, ent_pos(e));
            ent_set_rot(e2, &e->rot);
            break;
        }
        case 16: {
            e = func_0200fdd8(rec->u.w[0]);
            func_020370ec(&params);
            params.kind = 0x6b;
            params.f10  = 1;
            ent_set_roty(e, rec->u.ang.a);
            p = func_0205e80c(0x10, 1, 4);
            if (p != NULL) {
                p = func_020363f4(p, e, rec->u.sb[4]);
            }
            func_0202c1a8(e->list, p);
            p = func_0205e80c(0x28, 1, 4);
            if (p != NULL) {
                p = func_02037128(p, e, &params);
            }
            func_0202c1a8(e->list, p);
            break;
        }
        case 17:
            e = func_0200fdd8(rec->u.w[0]);
            ent_set_roty(e, rec->u.ang.a);
            p = func_0205e80c(0x10, 1, 4);
            if (p != NULL) {
                p = func_020363f4(p, e, rec->u.sb[4]);
            }
            func_0202c1a8(e->list, p);
            break;
        case 18: {
            Sess **ms = sess_ref();
            if ((*ms)->f50 == 0) {
                break;
            }
            (*ms)->f50 = 0;
            if ((*ms)->f10 != NULL) {
                func_0209d150((*ms)->f10, (*ms)->f3c / 60);
            }
            data_021092f4->vt->f18(data_021092f4, 0);
            func_0205ecc4(&data_02102900, 0x2c, 1, 0, 0, 0);
            func_0205ecc4(&data_02102900, 0x2b, 0, 0, 0, 0);
            break;
        }
        case 19: {
            OSOwnerInfo info;
            NameMsg msg;
            if (node_kind(data_02102900.f4) == 0x22) {
                func_0205ecc4(&data_02102900, 0x27, 1, 0, 0, 0);
                func_0205ecc4(&data_02102900, 0xf, 0, 0, 0, 0);
            }
            func_020730d0(rec->u.b[1]);
            func_02073054(rec->u.b[0]);
            func_0205ce5c(&data_021027f8, rec->u.b[0]);
            OS_GetOwnerInfo(&info);
            for (i = 0; i < info.nickNameLength; i++) {
                msg.name[i] = info.nickName[i];
            }
            for (; i < 10; i++) {
                msg.name[i] = 0;
            }
            msg.id = data_021092f4->vt->f1c(data_021092f4);
            func_020b04f0(data_0210794c, 0x14, &msg, sizeof(msg));
            break;
        }
        case 20:
            func_0209bcf8(data_02104d90.player, idx, &rec->u.b[4]);
            break;
        case 61:
            for (j = 0; j < 3; j++) {
                for (i = 0; i < 5; i++) {
                    data_02104d90.slots[idx][j].h[i] = rec->u.slots.h[j][i];
                }
                data_02104d90.slots[idx][j].b = rec->u.slots.b[j];
            }
            break;
        case 67:
            for (k = 0; k < 12; k++) {
                if (rec->u.b[k] != 100) {
                    func_02086458(&data_02104d90, rec->u.b[k]);
                }
            }
            break;
        case 62:
            func_020a3458(func_020aaeb4(), 1);
            break;
        case 63:
            func_0208fa1c(data_0210930c->world, 0, rec->u.b[1], rec->u.b[0]);
            break;
        case 21:
            func_02093c6c(data_0210930c, 0);
            if (data_021092f4->vt->f1c(data_021092f4) == 0) {
                func_02088b84(data_02104df4, func_020917b4(data_0210930c));
                func_0205ecc4(&data_02102900, 4, 0, 0xe, func_020917b4(data_0210930c), 0);
                func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
            } else {
                func_0205ecc4(&data_02102900, 4, 0, 0x11, 0xe, 0);
                func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
            }
            if (data_021027f8.local_id == 0) {
                func_020b111c(data_02104d90.menu, is_remote(idx));
            } else {
                func_02072bc8();
            }
            break;
        case 22:
        case 23:
        case 24:
            e = func_0200fdd8(rec->u.w[0]);
            if (e == NULL || (ent_flags(e) & 2) == 2 || (ent_flags(e) & 0x800) == 0x800) {
                break;
            }
            if (e->mode & 3) {
                e->f6a0.x = rec->u.w[1];
                e->f6a0.y = rec->u.w[2];
                e->f6a0.z = rec->u.w[3];
                if (rec->type == 0x18) {
                    func_02004b78(e, 0x29);
                } else if (rec->type == 0x17) {
                    e->vt->f40(e, 0);
                } else {
                    e->flags |= 2;
                }
            } else if (rec->type != 0x18 && rec->type != 0x17) {
                e->flags |= 2;
            }
            break;
        case 25:
            func_02090124(data_0210930c->world, 7, &rec->u);
            break;
        case 27:
            p = func_02096adc(data_0210930c, rec->u.w[0]);
            if (p != NULL) {
                func_0209678c(data_0210930c, p);
            }
            break;
        case 26:
            if (rec->u.h[0] != 0) {
                func_0206763c(data_0210930c->world->scene->cam->part, rec->u.h[0]);
            }
            if (rec->u.b[2] != 0) {
                func_020675d8(data_0210930c->world->scene->cam->part, rec->u.b[2]);
            }
            break;
        case 28: {
            Xform xf;
            Sess **ms;
            o = (Ent *) func_0205e80c(0x6f8, 0, 4);
            if (o != NULL) {
                xform_init(&xf);
                o = func_020029c4(o, 2, 0x17, 0, &xf);
            }
            ms    = sess_ref();
            m     = *ms;
            v28.x = 0;
            v28.y = 0;
            v28.z = 0;
            vec_set(&v28, &m->pos);
            ent_set_pos(o, &v28);
            (*ms)->f6  = 1;
            (*ms)->f8  = 0;
            (*ms)->f42 = 0;
            func_0209bd50(*ms, -1);
            break;
        }
        case 29: {
            Xform xf;
            Sess **ms;
            func_02001688(0);
            ms   = sess_ref();
            slot = (*ms)->f41;
            if (slot != -1) {
                func_02006220(data_02109358[slot], 0);
                o = (Ent *) func_0205e80c(0x6f8, 0, 4);
                if (o != NULL) {
                    xform_init(&xf);
                    o = func_020029c4(o, 2, 0x17, 0, &xf);
                }
                m     = *ms;
                v29.x = 0;
                v29.y = 0;
                v29.z = 0;
                vec_set(&v29, &m->pos);
                ent_set_pos(o, &v29);
                if ((*ms)->f10 != NULL) {
                    func_0209d038((*ms)->f10, (u8) slot);
                }
                func_0209bce0(*ms, (u8) slot, 5);
                (*ms)->f6 = 1;
                (*ms)->f8 = 0;
                func_0209bd50(*ms, -1);
                (*ms)->f42 = 0;
            }
            func_020016c0();
            break;
        }
        case 30: {
            Xform xf;
            Sess **ms = sess_ref();
            m         = *ms;
            if (m->f8 != 0 || m->fc != 0) {
                break;
            }
            m->fc = 1;
            vec_make(&v30, rec->u.v.x, rec->u.v.y, rec->u.v.z);
            o = (Ent *) func_0205e80c(0x6f8, 0, 4);
            if (o != NULL) {
                xform_init(&xf);
                o = func_020029c4(o, 2, 0x17, 0, &xf);
            }
            if ((*ms)->f6 == 0) {
                ent_set_pos(o, &v30);
            } else {
                q = func_02066ff0(data_0210930c->world->scene, 0xc, 0);
                ent_place(o, q, &t30, &r30);
                (*ms)->f6 = 0;
            }
            func_0209bbf0(*ms);
            (*ms)->f42 = 0;
            func_0209bd50(*ms, -1);
            break;
        }
        case 31: {
            Sess **ms = sess_ref();
        // FAKE: always-true null test keeps ms out of copy propagation (address held in a register)
            if (ms) {
                (*ms)->f3c = rec->u.w[0];
                func_0209b6d4(*ms, rec->u.b[4]);
                (*ms)->f40 = rec->u.b[5];
            }
            break;
        }
        case 32:
            data_02101ad4 = rec->u.w[0];
            break;
        case 33:
            i = func_02093fd0(func_020917b4(data_0210930c));
            if (i != func_02093fd0(rec->u.b[0])) {
                func_02088b78(data_02104df4);
                data_02104d90.f50 = 0;
                data_02104d90.f51 = -1;
            } else {
                func_02088b84(data_02104df4, func_020917b4(data_0210930c));
            }
            func_0206dd60();
            func_0205ecc4(&data_02102900, 0x1e, 1, 0, 0, 0);
            func_0205ecc4(&data_02102900, 4, 0, 4, rec->u.b[0], rec->u.b[1]);
            break;
        case 34:
            if (rec->u.b[0] != 0) {
                for (n = 0; n < 4; n++) {
                    for (k = 0; k < 5; k++) {
                        data_02104d90.hud[n].bars[k] = 250;
                    }
                    data_02104d90.hud[n].level = 100;
                    data_02104d90.hud[n].f1b   = 0;
                }
                if (rec->u.b[1] != 100) {
                    data_02104d90.hud[0].level = rec->u.b[1];
                    data_02104d90.hud[0].f1b   = 1;
                }
                if (rec->u.b[2] != 100) {
                    data_02104d90.hud[1].level = rec->u.b[2];
                    data_02104d90.hud[1].f1b   = 1;
                }
                if (rec->u.b[3] != 100) {
                    data_02104d90.hud[2].level = rec->u.b[3];
                    data_02104d90.hud[2].f1b   = 1;
                }
                if (rec->u.b[4] != 100) {
                    data_02104d90.hud[3].level = rec->u.b[4];
                    data_02104d90.hud[3].f1b   = 1;
                }
            }
            data_02102900.f58 = rec->u.b[5];
            break;
        case 35: {
            vec_make(&v35, rec->u.v.x, rec->u.v.y, rec->u.v.z);
            func_020aa890(func_020aaeb4(), &v35);
            break;
        }
        case 36:
            func_0206769c(data_0210930c->world->scene->cam->part, rec->u.w[0], rec->u.w[1]);
            break;
        case 37:
            p = data_02109358[rec->u.b[0]]->f5f8;
            if (p == NULL || ((Sub *) p)->fe != 1) {
                break;
            }
            func_02048a54(p, rec->u.b[1]);
            break;
        case 42:
            func_020922f4(data_0210930c, rec->u.b[0], rec->u.b[1]);
            break;
        case 43:
            func_0208ce8c(data_02102900.f0);
            break;
        case 44:
            func_02093c6c(data_0210930c, 0);
            func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
            break;
        case 45:
            func_02093c6c(data_0210930c, 0);
            if (idx == 0 || idx == data_021027f8.local_id) {
                func_0205ecc4(&data_02102900, 4, 0, 0x11, 0xe, 0);
                func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
            } else {
                func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
            }
            if (data_021027f8.local_id == 0) {
                func_020b111c(data_02104d90.menu, 1);
            } else {
                func_02072bc8();
            }
            break;
        case 46:
            e = data_02109358[rec->u.b[0]];
            if (e != NULL) {
                func_0207b370(e, rec->u.b[1], rec->u.b[2]);
            }
            break;
        case 47:
            e = data_02109358[rec->u.b[0]];
            if (e != NULL) {
                func_0207b3bc(e, rec->u.b[1]);
            }
            break;
        case 48:
            func_020861bc(&data_02104d90, 10, 0);
            func_0205ecc4(&data_02102900, 0xf, 0, 0, 0, 0);
            func_0205ecc4(&data_02102900, 0x1c, 1, rec->u.b[2], rec->u.b[3], rec->u.h[0]);
            break;
        case 49:
            func_020861bc(&data_02104d90, 10, 1);
            func_02088b78(data_02104df4);
            data_02104d90.f50          = 0;
            data_02104d90.f51          = -1;
            data_02104d90.hud[0].level = data_0210930c->hero[0]->f6bb;
            data_02104d90.hud[1].level = data_0210930c->hero[1]->f6bb;
            func_0205ecc4(&data_02102900, 4, 0, 4, rec->u.b[2], rec->u.h[0]);
            func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
            break;
        case 50:
            if (node_kind(data_02102900.f0) != 4) {
                break;
            }
            func_02060158(&data_02102900, 0, 0x19, 1, rec->u.b[0]);
            break;
        case 51:
            if (data_02102900.f4->kind == 0x1c) {
                break;
            }
            data_02104d90.flag0 = 1;
            for (k = 0; k < data_021027f8.count; k++) {
                ((HeroObj *) data_0210930c->hero[k])->action(0, 0, 1, 0);
                vec_set(&data_0210930c->hero[k]->f60, &data_020ff9b4);
            }
            func_02060158(&data_02102900, 1, 0x17, 5, rec->u.b[4], rec->u.b[5], rec->u.b[6], rec->u.h[0], rec->u.h[1]);
            break;
        case 52: {
            Xform xf;
            e = func_02090668(data_0210930c->world, rec->u.h[0], 0);
            o = (Ent *) func_0205e80c(0x6f8, 0, 4);
            if (o != NULL) {
                xform_at(&xf, (s16 **) e, &scale, &rot);
                o = func_020029c4(o, 2, rec->u.b[2], 0, &xf);
            }
            func_02093690(data_0210930c, o);
            func_02004b78(o, 0x22);
            break;
        }
        case 53:
            if (func_02060198(&data_02102900) != 0) {
                break;
            }
            func_020601ac(&data_02102900, 0, data_020f79c0[rec->u.w[0]], -1, 4, 1);
            break;
        case 54:
            func_02086810(&data_02104d90, rec->u.w[0], 1);
            break;
        case 55:
            if (node_kind(data_02102900.f4) == 0x1e) {
                break;
            }
            data_02104d90.f28d1 = rec->u.b[0xd];
            {
                // FAKE: pointer locals give 0x02107662/0x02107663 their own pool words (no symbols.txt split; natural source
                // is two u8 globals)
                u8 *pe;
                u8 *pf;

                pe  = &data_02107654[0xe];
                *pe = rec->u.b[0xe];
                pf  = &data_02107654[0xf];
                *pf = 0;
                if (*pe == 0xf) {
                    *pf = 1;
                }
            }
            func_02097cb8(rec->u.h[0], rec->u.h[1], rec->u.w[1], rec->u.w[2], rec->u.b[0xc]);
            break;
        case 56: {
            e = func_0200fdd8(rec->u.w[0]);
            vec_make(&v56, rec->u.idv.v.x, rec->u.idv.v.y, rec->u.idv.v.z);
            vec_set(&e->f6a0, &v56);
            break;
        }
        case 57:
            func_020a9a24(func_020aaeb4(), rec->u.b[1], rec->u.b[0]);
            break;
        case 58: {
            Xform xf;
            vec_make(&v58, rec->u.v.x, rec->u.v.y, rec->u.v.z);
            xform_make(&xf, &v58, &scale, &rot);
            p = func_02015138(0x1b, 0, &xf);
            func_02015dcc(p, 0, rec->u.b[0xc], 0, 0, 0);
            func_0208fec8(data_0210930c->world, p);
            break;
        }
        case 59:
            func_020871d8(&data_02104d90, rec->u.sb[0]);
            break;
        case 60:
            func_02093c6c(data_0210930c, 0);
            func_0208642c(&data_02104d90);
            func_0208896c(data_02104df4);
            for (i = 0; i < 2; i++) {
                if (data_0210930c->hero[i] != NULL && func_02086534(&data_02104d90, data_0210930c->hero[i]->f6bb) != 0 &&
                    data_0210930c->hero[i]->f6bb < 0x54) {
                    data_02104d90.hud[i].level = data_0210930c->hero[i]->f6bb;
                    data_02104d90.hud[i].f1a   = 4;
                    data_02104d90.hud[i].f1b   = 1;
                }
            }
            func_0205ecc4(&data_02102900, 4, 0, 4, 0xe, 0);
            func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
            break;
        case 64:
            func_0207d8f8(data_02102900.f4 + 0x28 / sizeof(Node));
            break;
        case 65:
            func_0207da48(data_02102900.f4 + 0x28 / sizeof(Node));
            break;
        case 66:
            i = data_021027f8.local_id;
            func_02093c6c(data_0210930c, 0);
            if (data_02104d90.menu->f6 != 0) {
                func_020b111c(data_02104d90.menu, 1);
            } else {
                func_02072bc8();
            }
            if (i == idx) {
                func_0205ecc4(&data_02102900, 0x11, 0, 0, 0, 0);
                func_0205ecc4(&data_02102900, 0xf, 1, 0, 0, 0);
            } else {
                func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
            }
            break;
        case 68:
            data_021090d0[rec->u.b[0]] = func_020aae08(func_020aaeb4(), rec->u.b[0]);
            break;
        case 69:
            j = rec->u.b[0];
            func_020a9a24(func_020aaeb4(), data_021090d0[j], j);
            break;
    }
}
