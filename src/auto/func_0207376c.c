#include "m2c.h"

typedef struct {
    u16 v[5];
} Rec;

typedef struct {
    Rec recs[3];
    u8 tail[3];
    u8 pad[0x5C - 0x21];
} Buf;

typedef struct {
    u16 offset;
    u16 size;
    u8 data[0x58];
} Packet;

typedef struct {
    u8 a, b, c, d, e, f;
} Ent;

typedef struct {
    u8 pad[0x52];
    Ent ents[3];
} Tbl;

extern u32 data_02102838;
extern u32 data_02102840;
extern u8 data_02102900[];
extern Tbl data_02104d90;
extern u8 data_02106d90;
extern u8 data_0210794c[];
extern void **data_021092f4;
extern M2C_UNK data_0210930c;
extern s32 data_027e02a8;

extern void func_02001688(s32 arg0);
extern M2C_UNK func_020016c0();
extern s32 func_0205e818(s32, M2C_UNK, M2C_UNK);
extern void func_0205e860(s32 arg0);
extern void func_0205ecc4(void *arg0, M2C_UNK arg1, M2C_UNK arg2, M2C_UNK arg3, s32 arg4, s32 arg5);
extern M2C_UNK func_02087044(void *);
extern void func_02093c6c(s32 arg0, s32 arg1);
extern void func_020b04f0(void *obj, u8 id, void *src, u8 size);
extern void func_020cf544();
extern void *memcpy(void *dst, const void *src, int n);

/* Handles one message of kind arg1 for the object arg0, with payload arg2,
 * bracketed by func_02001688(1) / func_020016c0().
 * 6: if the slot-0x1C virtual on *data_021092f4 is nonzero, keeps the 32-bit
 *    payload word at +0xA4 and stores func_0205e818(word, 3, 4) at +0xA0.
 * 2: same guard; copies a 0x5C-byte packet {offset, size, data} and writes
 *    `size` data bytes to the +0xA0 buffer at `offset`, adding size to the
 *    u16 at +0xE0AC.
 * 3: hands the +0xA0 buffer to func_020cf544 and func_0205e860 (which skips
 *    NULL), clears +0xA0, sets the byte at data_02106d90+0xC28, zeroes the
 *    two counters, packs the three 6-byte data_02104d90 entries into a
 *    0x5C-byte record (tag 0x3D) for func_020b04f0, then sends the
 *    func_0205ecc4 pair (kind 0x11 instead of 0xE when the +0x9C byte at
 *    0x286C was 0xD, which is then set to 0xE). */
void func_0207376c(u8 *arg0, s32 arg1, u8 *arg2) {
    Packet pkt;
    Buf buf;
    u8 i;
    s32 type;
    s32 value;

    func_02001688(1);
    switch (arg1) {
        case 6:
            if (M2C_FIELD(*data_021092f4, s32(**)(), 0x1C)(data_021092f4) != 0) {
                value                        = M2C_FIELD(arg2, s32 *, 0);
                M2C_FIELD(arg0, s32 *, 0xA4) = value;
                M2C_FIELD(arg0, s32 *, 0xA0) = func_0205e818(value, 3, 4);
            }
            break;
        case 2:
            if (M2C_FIELD(*data_021092f4, s32(**)(), 0x1C)(data_021092f4) != 0) {
                memcpy(&pkt, arg2, 0x5c);
                memcpy(M2C_FIELD(arg0, u8 **, 0xA0) + pkt.offset, pkt.data, pkt.size);
                M2C_FIELD(arg0, u16 *, 0xE0AC) += pkt.size;
            }
            break;
        case 3:
            func_020cf544(M2C_FIELD(arg0, u8 **, 0xA0), M2C_FIELD(arg0, u8 **, 0x9C));
            func_0205e860(M2C_FIELD(arg0, s32 *, 0xA0));
            M2C_FIELD(arg0, s32 *, 0xA0)           = 0;
            M2C_FIELD(&data_02106d90, u8 *, 0xC28) = 1;
        /* frame counters polled by the wait loop in func_020013a4 */
            data_02102838                    = 0;
            *(volatile u32 *) &data_02102840 = 0; // FAKE: volatile frame-counter store keeps literal loads in target slots
            for (i = 0; i < 3; i++) {
                buf.recs[i].v[4] = data_02104d90.ents[i].e;
                buf.recs[i].v[0] = data_02104d90.ents[i].a;
                buf.recs[i].v[3] = data_02104d90.ents[i].d;
                buf.recs[i].v[1] = data_02104d90.ents[i].c;
                buf.recs[i].v[2] = data_02104d90.ents[i].b;
                buf.tail[i]      = data_02104d90.ents[i].f;
            }
            func_020b04f0(data_0210794c, 0x3D, &buf, 0x5C);
            if (data_027e02a8 & 3) {
                func_02093c6c((s32) &data_0210930c, 0);
                func_02087044(&data_02104d90);
            }
            type = 0xE;
            if (M2C_FIELD(arg0, u8 **, 0x9C)[0x286C] == 0xD) {
                M2C_FIELD(arg0, u8 **, 0x9C)[0x286C] = 0xE;
                type                                 = 0x11;
            }
            if (M2C_FIELD(*data_021092f4, s32(**)(), 0x1C)(data_021092f4) != 0) {
                func_0205ecc4(data_02102900, 4, 0, type, M2C_FIELD(arg0, u8 **, 0x9C)[0x286C], 0);
                func_0205ecc4(data_02102900, 0x30, 1, 0, 0, 0);
                M2C_FIELD(arg0, u16 *, 0xE0B2) = M2C_FIELD(arg2, u16 *, 4);
            } else {
                func_0205ecc4(data_02102900, 4, 0, type, M2C_FIELD(arg0, u8 **, 0x9C)[0x286C], 0);
                func_0205ecc4(data_02102900, 0x30, 1, 0, 0, 0);
            }
            break;
    }
    func_020016c0();
}
