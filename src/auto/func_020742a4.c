#include "m2c.h"

extern u8 data_02102900[];

typedef struct {
    u8 pad0[0x30];
    u8 sub30[0x10];
    u8 sub40[0x69];
    u8 mode;
} Obj;

void func_0205ecc4(void *, s32, s32, s32, s32, s32);
void func_0205365c(void *, s32);
void *func_0205e80c(s32, s32, s32);
void *func_02069350(void *, void *, void *, void *, s32);
void *func_0206afa8(void *, void *, void *, void *);
void func_0205eb6c(void *, void *, s32);

void func_020742a4(Obj *this, s32 kind) {
    void *p;

    switch (kind) {
        case 0:
            func_0205ecc4(data_02102900, 4, 0, 5, 14, 0);
            func_0205ecc4(data_02102900, 0x30, 1, 0, 0, 0);
            break;
        case 1:
            func_0205365c(this->sub30, 0);
            this->mode = 3;
            p          = func_0205e80c(0x2c, 3, 4);
            if (p != 0) {
                p = func_02069350(p, this, this->sub40, this->sub30, 0);
            }
            func_0205eb6c(this, p, 0);
            break;
        case 2:
            func_0205365c(this->sub30, 0);
            this->mode = 0;
            p          = func_0205e80c(0x50, 3, 4);
            if (p != 0) {
                p = func_0206afa8(p, this, this->sub40, this->sub30);
            }
            func_0205eb6c(this, p, 0);
            break;
    }
}
