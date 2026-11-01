/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* WriteHexFloatDigits from https://github.com/tylzars/bakubrawlers libs/msl/src/msl_printf.c (license: none), reused by
 * tools/donor_match.py */
typedef struct FmtSpec {
    unsigned char justify;
    unsigned char sign;
    unsigned char prec_set;
    unsigned char alt;
    unsigned char len;
    unsigned char conv;
    int width;
    int prec;
} FmtSpec;
typedef struct DecDigits {
    char sign;
    unsigned char unk1;
    short exp;
    unsigned char len;
    unsigned char text[35];
} DecDigits;
typedef struct DecForm {
    unsigned char mode;
    short ndigits;
} DecForm;
char *long2str(int val, char *end, FmtSpec spec);
typedef struct B4 {
    char c[4];
} B4;
typedef struct B5 {
    char c[5];
} B5;
typedef struct B6 {
    char c[6];
} B6;
extern const B6 data_02101890;
extern const B5 data_02101898, data_021018a0, data_021018b0, data_021018b8;
extern const B4 data_021018a8, data_021018ac, data_021018c0, data_021018c4;
extern void __num2dec(DecForm *, double, DecDigits *);
extern double fabs(double);
char *double2hex(double x, char *end, FmtSpec spec) {
    char *p = end;
    DecForm f;
    FmtSpec e;
    DecDigits d;
    unsigned char *xb;
    int i, n, bit, ex;
    double ld;
    unsigned snum;
    char *q;
    ld = x;
    if (spec.prec > 509) {
        return 0;
    }
    f.mode    = 0;
    f.ndigits = 32;
    __num2dec(&f, ld, &d);
    if (0.0 == fabs(x)) {
        p = end - 6;
        __memcpy(p, &data_02101890, 6);
        return p;
    }
    if (d.text[0] == 'I') {
        if (d.sign) {
            p = end - 5;
            if (spec.conv == 'A') {
                __memcpy(p, &data_02101898, 5);
            } else {
                __memcpy(p, &data_021018a0, 5);
            }
        } else {
            p = end - 4;
            if (spec.conv == 'A') {
                __memcpy(p, &data_021018a8, 4);
            } else {
                __memcpy(p, &data_021018ac, 4);
            }
        }
        return p;
    }
    if (d.text[0] == 'N') {
        if (d.sign) {
            p = end - 5;
            if (spec.conv == 'A') {
                __memcpy(p, &data_021018b0, 5);
            } else {
                __memcpy(p, &data_021018b8, 5);
            }
        } else {
            p = end - 4;
            if (spec.conv == 'A') {
                __memcpy(p, &data_021018c0, 4);
            } else {
                __memcpy(p, &data_021018c4, 4);
            }
        }
        return p;
    }
    e.justify  = 1;
    e.sign     = 1;
    e.prec_set = 0;
    e.alt      = 0;
    e.len      = 0;
    e.width    = 0;
    e.prec     = 1;
    e.conv     = 'd';
    q          = (char *) &x;
    for (i = 0; i < 4; i++) {
        char t   = q[i];
        q[i]     = q[7 - i];
        q[7 - i] = t;
    }
    snum = ((unsigned char *) &x)[0] << 25;
    snum |= ((unsigned char *) &x)[1] << 17;
    snum = (snum >> 21) & 0x7ff;
    ex   = snum - 1023;
    p    = long2str(ex, p, e);
    if (spec.conv == 'a') {
        *--p = 'p';
    } else {
        *--p = 'P';
    }
    xb  = (unsigned char *) &x;
    bit = spec.prec * 4 + 11;
    for (n = spec.prec; n >= 1; n--) {
        unsigned char ch;
        if (bit < 64) {
            unsigned char v = xb[bit >> 3] >> (7 - (bit & 7));
            if ((bit & ~7) != ((bit - 4) & ~7)) {
                v = (v | (xb[(bit >> 3) - 1] << 8) >> (7 - (bit & 7))) & 0xff;
            }
            v &= 0xf;
            if (v < 10) {
                ch = v + '0';
            } else if (spec.conv == 'a') {
                ch = v + 'a' - 10;
            } else {
                ch = v + 'A' - 10;
            }
        } else {
            ch = '0';
        }
        *--p = ch;
        bit -= 4;
    }
    if (spec.prec != 0 || spec.alt != 0) {
        *--p = '.';
    }
    *--p = '1';
    if (spec.conv == 'a') {
        *--p = 'x';
    } else {
        *--p = 'X';
    }
    *--p = '0';
    if (d.sign) {
        *--p = '-';
    } else if (spec.sign == 1) {
        *--p = '+';
    } else if (spec.sign == 2) {
        *--p = ' ';
    }
    return p;
}
