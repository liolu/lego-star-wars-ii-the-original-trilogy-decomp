#include "m2c.h"

typedef struct {
    u8 a;
    u8 b;
} Pair;

extern u16 *data_020ff4cc[];
extern u16 *data_020ff4dc[];
extern Pair data_020f1f9c[];

u32 func_0208d8cc(u16 arg0, u16 arg1) {
    u16 i;
    u16 j;
    u16 k;
    u16 m;
    u16 j2;
    s16 a;
    s16 b;

    for (i = 0; i < 4; i++) {
        a = -1;
        for (j = 0; data_020ff4cc[i][j] != 0x28; j++) {
            if (arg0 == data_020ff4cc[i][j]) {
                a = i;
                break;
            }
        }
        for (k = 0; k < 7; k++) {
            b = -1;
            for (j2 = 0; data_020ff4dc[k][j2] != 0xFA; j2++) {
                if (arg1 == data_020ff4dc[k][j2]) {
                    b = k;
                    break;
                }
            }
            if (a != -1 && b != -1) {
                m = 0;
                do {
                    if (a == data_020f1f9c[m].a && b == data_020f1f9c[m].b) {
                        return 0;
                    }
                    m++;
                } while (data_020f1f9c[m].a != 4);
            }
        }
    }
    return 1;
}
