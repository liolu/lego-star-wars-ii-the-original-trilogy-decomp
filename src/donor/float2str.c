/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* func_0201368c from https://github.com/tylzars/bakubrawlers libs/msl/src/msl_printf_fmt.c (license: none), reused by
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
extern const unsigned short __ctype_mapC[];
typedef struct B4 {
    char c[4];
} B4;
typedef struct B5 {
    char c[5];
} B5;
extern const B5 data_02101898, data_021018a0, data_021018b0, data_021018b8;
extern const B4 data_021018a8, data_021018ac, data_021018c0, data_021018c4;
extern void __num2dec(DecForm *, double, DecDigits *);
extern void round_decimal(DecDigits *, int);
char *float2str(double x, char *end, FmtSpec spec) {
    int sign;
    int alt;
    int prec;
    int conv;
    char *p;
    char *q;
    DecForm f;
    DecDigits d;
    int ex, n, i, k;
    char esign;
    int whole;
    int frac;
    alt  = spec.alt;
    sign = spec.sign;
    prec = spec.prec;
    conv = spec.conv;
    if (prec > 509) {
        return 0;
    }
    f.mode    = 0;
    f.ndigits = 32;
    __num2dec(&f, x, &d);
    q = d.text + d.len;
    while (d.len > 1 && *--q == '0') {
        d.len--;
        d.exp++;
    }
    switch (d.text[0]) {
        case '0':
            d.exp = 0;
            break;
        case 'I':
            if (x < 0.0) {
                p = end - 5;
                if ((((conv) < 0 || (conv) >= 128) ? 0 : (__ctype_mapC[conv] & 0x200))) {
                    __memcpy(p, &data_02101898, 5);
                } else {
                    __memcpy(p, &data_021018a0, 5);
                }
            } else {
                p = end - 4;
                if ((((conv) < 0 || (conv) >= 128) ? 0 : (__ctype_mapC[conv] & 0x200))) {
                    __memcpy(p, &data_021018a8, 4);
                } else {
                    __memcpy(p, &data_021018ac, 4);
                }
            }
            return p;
        case 'N':
            if (d.sign) {
                p = end - 5;
                if ((((conv) < 0 || (conv) >= 128) ? 0 : (__ctype_mapC[conv] & 0x200))) {
                    __memcpy(p, &data_021018b0, 5);
                } else {
                    __memcpy(p, &data_021018b8, 5);
                }
            } else {
                p = end - 4;
                if ((((conv) < 0 || (conv) >= 128) ? 0 : (__ctype_mapC[conv] & 0x200))) {
                    __memcpy(p, &data_021018c0, 4);
                } else {
                    __memcpy(p, &data_021018c4, 4);
                }
            }
            return p;
    }
    p = end;
    d.exp += d.len - 1;
    *--p = 0;
    switch (conv) {
        case 'g':
        case 'G':
            if (d.len > prec) {
                round_decimal(&d, prec);
            }
            if (d.exp < -4 || d.exp >= prec) {
                if (alt) {
                    prec--;
                } else {
                    prec = d.len - 1;
                }
                if (spec.conv == 'g') {
                    spec.conv = 'e';
                } else {
                    spec.conv = 'E';
                }
                goto do_e;
            } else {
                if (alt) {
                    prec -= d.exp + 1;
                } else {
                    prec = d.len - (d.exp + 1);
                    if (prec < 0) {
                        prec = 0;
                    }
                }
                goto do_f;
            }
        case 'e':
        case 'E':
        do_e:
            if (d.len > prec + 1) {
                round_decimal(&d, prec + 1);
            }
            ex    = d.exp;
            esign = '+';
            n     = 0;
            if (ex < 0) {
                ex    = -ex;
                esign = '-';
            }
            while (ex != 0 || n < 2) {
                *--p = ex % 10 + '0';
                ex /= 10;
                n++;
            }
            *--p = esign;
            *--p = spec.conv;
            if ((end - p) + prec > 509) {
                return 0;
            }
            if (d.len < prec + 1) {
                for (i = prec + 2 - d.len; --i != 0;) {
                    *--p = '0';
                }
            }
            q = d.text + d.len;
            for (i = d.len - 1; i != 0; i--) {
                *--p = *--q;
            }
            if (prec != 0 || alt != 0) {
                *--p = '.';
            }
            *--p = d.text[0];
            if (d.sign) {
                *--p = '-';
            } else if (sign == 1) {
                *--p = '+';
            } else if (sign == 2) {
                *--p = ' ';
            }
            break;
        case 'f':
        case 'F':
        do_f:
            frac = d.len - d.exp - 1;
            if (frac < 0) {
                frac = 0;
            }
            if (frac > prec) {
                round_decimal(&d, d.len - (frac - prec));
                frac = d.len - d.exp - 1;
                if (frac < 0) {
                    frac = 0;
                }
            }
            whole = d.exp + 1;
            if (whole < 0) {
                whole = 0;
            }
            if (whole + frac > 509) {
                return 0;
            }
            q = d.text + d.len;
            for (i = 0; i < prec - frac; i++) {
                *--p = '0';
            }
            for (i = 0; i < frac && i < d.len; i++) {
                *--p = *--q;
            }
            for (; i < frac; i++) {
                *--p = '0';
            }
            if (prec != 0 || alt != 0) {
                *--p = '.';
            }
            if (whole != 0) {
                for (k = 0; k < whole - d.len; k++) {
                    *--p = '0';
                }
                for (; k < whole; k++) {
                    *--p = *--q;
                }
            } else {
                *--p = '0';
            }
            if (d.sign) {
                *--p = '-';
            } else if (sign == 1) {
                *--p = '+';
            } else if (sign == 2) {
                *--p = ' ';
            }
            break;
    }
    return p;
}
extern char *float2str(double, char *, FmtSpec);
