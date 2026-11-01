#include "m2c.h"

typedef struct Unk01ffb658 {
    s32 count;                              /* 0x0: bumped once per call */
    s32 unk4;
    s32 unk8;
    void (*callback)(struct Unk01ffb658 *); /* 0xc: optional, gets the record */
} Unk01ffb658;

/*
 * Runs the record's optional callback (passing the record itself), then
 * increments the record's counter whether or not a callback was set.
 * Reached only through the pointer table data_02101498.
 */
void func_01ffb658(Unk01ffb658 *arg0) {
    if (arg0->callback != 0) {
        arg0->callback(arg0);
    }
    arg0->count++;
}
