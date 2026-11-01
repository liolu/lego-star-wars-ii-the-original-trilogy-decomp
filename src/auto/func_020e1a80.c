/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
typedef struct decimal {
    char sgn;
    char unused;
    short exp;
    struct {
        unsigned char length;
        unsigned char text[32];
        unsigned char unused;
    } sig;
} decimal;

extern int func_020e1d3c(double x);
extern int __fpclassifyd(double x);
extern double frexp(double x, int *eptr);
extern double ldexp(double x, int n);
extern int func_020e1ca8(unsigned long long x);
extern void func_020e16fc(decimal *result, long exp);
extern void __ull2dec(decimal *result, unsigned long long x);
extern void __timesdec(decimal *result, const decimal *x, const decimal *y);
/* Runtime helpers still unnamed in symbols.txt: _dsub (a - b) and _ll_ufrom_d (double -> u64). */
extern double _dsub(double a, double b);
extern unsigned long long _ll_ufrom_d(double x);
void func_020e1a80(decimal *d, double x);

enum {
    FPCLASS_NAN      = 1,      /* __fpclassifyd: exponent all ones, nonzero mantissa */
    FPCLASS_INFINITE = 2, /* __fpclassifyd: exponent all ones, zero mantissa */
    DOUBLE_MANT_BITS = 53 /* significand bits of an IEEE double, implicit bit included */
};

/* Trailing zero bits of x: popcount (func_020e1ca8) of the mask below its lowest set bit. */
static inline int count_trailing_zerol(unsigned long long x) {
    return func_020e1ca8((x & -x) - 1);
}

/* Trailing zero bits of x's raw bit pattern, with the implicit mantissa bit (bit 52) forced on. */
static inline int count_trailing_zero(double x) {
    unsigned long long y = *(unsigned long long *) &x;
    return count_trailing_zerol(y | 0x0010000000000000ULL);
}

/*
 * Exact binary-to-decimal conversion of x into d (sign, exponent, digit values 0-9), used by __num2dec.
 * Zero gives one digit 0; NaN and infinity give one digit 'N' or 'I' with exponent 0.
 * Otherwise |x| is split by frexp into a mantissa and power of two; the mantissa is scaled to the
 * smallest integer holding all its set bits, converted with __ull2dec, and multiplied (__timesdec)
 * by the decimal value of the remaining power of two (func_020e16fc). The sign is stored last.
 */
void func_020e1a80(decimal *d, double x) {
    char sgn = (char) (func_020e1d3c(x) != 0);

    if (x == 0.0) {
        d->sgn         = sgn;
        d->exp         = 0;
        d->sig.length  = 1;
        d->sig.text[0] = 0;
        return;
    }

    if (__fpclassifyd(x) <= FPCLASS_INFINITE) {
        d->sgn         = sgn;
        d->exp         = 0;
        d->sig.length  = 1;
        d->sig.text[0] = (__fpclassifyd(x) == FPCLASS_NAN) ? 'N' : 'I';
        return;
    }

    if (sgn) {
        x = _dsub(0.0, x);
    }

    {
        int exp;
        double frac           = frexp(x, &exp);
        long num_bits_extract = DOUBLE_MANT_BITS - count_trailing_zero(frac);
        decimal int_d, pow2_d;

        func_020e16fc(&pow2_d, exp - num_bits_extract);
        __ull2dec(&int_d, _ll_ufrom_d(ldexp(frac, num_bits_extract)));
        __timesdec(d, &int_d, &pow2_d);
        d->sgn = sgn;
    }
}
