/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* __pformatter from https://github.com/ctrl-cheeb-del/pokeheartgold lib/MSL_C/src/msl_printf_formatter.c (license: none),
 * reused by tools/donor_match.py */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short int s16;
typedef signed long s32;
typedef unsigned long long int u64;
typedef signed long long int s64;
typedef char *va_list;
typedef __typeof__(sizeof(0)) size_t;
extern void *memchr(const void *arr, int value, size_t size);
extern unsigned func_020e0db0(const char *str);
typedef struct Format {
    u8 justify;
    u8 sign;
    u8 precisionSet;
    u8 alternate;
    u8 length;
    u8 conversion;
    u8 pad[2];
    int width;
    int precision;
} Format;
typedef struct OutputWorkspace {
    Format info;
    char buffer[512];
} OutputWorkspace;
typedef u32 (*Writer)(void *context, const char *data, u32 count);
extern const char data_021018c8[];
extern const char data_021018cc[];
extern char *strchr(const char *, int);
extern unsigned int func_020e0db0(const char *);
extern void *memchr(const void *, int, u32);
extern unsigned int wcstombs(char *, const u16 *, unsigned int);
extern char *parse_format(char *start, va_list *args, Format *out);
extern char *long2str(long num, char *buf, Format f);
extern char *func_020df1fc(u64 value, char *end, Format f);
extern char *double2hex(double value, char *end, Format f);
extern char *float2str(double x, char *end, Format fmt);
extern int __pformatter(Writer write, void *context, const char *format, va_list ap);
char *parse_format(char *start, va_list *args, Format *out);
char *long2str(long num, char *buf, Format f);
char *func_020df1fc(u64 value, char *end, Format f);
char *double2hex(double value, char *end, Format f);
char *float2str(double x, char *end, Format fmt);
static inline char *Small(s32 n, char *end, const Format *f) {
    return long2str(n, end, *f);
}
static inline char *Large(u64 n, char *end, const Format *f) {
    return func_020df1fc(n, end, *f);
}
int __pformatter(Writer write, void *context, const char *format, va_list ap) {
    OutputWorkspace workspace;
    int field;
    char padding = ' ', space;
    double value;
    s64 wide;
    const char *percent;
    int length;
    register int number;
    register char *text;
    int count = 0;
    if (!*format) {
        goto done;
    }
    do {
        percent = strchr(format, '%');
        if (!percent) {
            length = func_020e0db0(format);
            count += length;
            if (length && !write(context, format, length)) {
                return -1;
            }
            break;
        }
        length = percent - format;
        count += length;
        if (length && !write(context, format, length)) {
            return -1;
        }
        format = parse_format((char *) percent, &ap, &workspace.info);
        switch (workspace.info.conversion) {
            case 'd':
            case 'i':
                if (workspace.info.length == 3) {
                    ap += 4;
                    number = *(long *) (ap - 4);
                } else if (workspace.info.length == 4) {
                    ap += 8;
                    wide = *(s64 *) (ap - 8);
                } else if (workspace.info.length == 6) {
                    ap += 8;
                    wide = *(s64 *) (ap - 8);
                } else if (workspace.info.length == 7) {
                    ap += 4;
                    number = *(s32 *) (ap - 4);
                } else if (workspace.info.length == 8) {
                    ap += 4;
                    number = *(u32 *) (ap - 4);
                } else {
                    ap += 4;
                    number = *(int *) (ap - 4);
                }
                if (workspace.info.length == 2) {
                    number = (s16) number;
                }
                if (workspace.info.length == 1) {
                    number = (s8) number;
                }
                if (workspace.info.length == 4 || workspace.info.length == 6) {
                    text = Large(wide, workspace.buffer + 512, &workspace.info);
                    if (!text) {
                        goto invalid;
                    }
                } else {
                    text = Small(number, workspace.buffer + 512, &workspace.info);
                    if (!text) {
                        goto invalid;
                    }
                }
                length = workspace.buffer + 511 - text;
                break;
            case 'o':
            case 'u':
            case 'x':
            case 'X':
                if (workspace.info.length == 3) {
                    ap += 4;
                    number = *(unsigned long *) (ap - 4);
                } else if (workspace.info.length == 4) {
                    ap += 8;
                    wide = *(u64 *) (ap - 8);
                } else if (workspace.info.length == 6) {
                    ap += 8;
                    wide = *(u64 *) (ap - 8);
                } else if (workspace.info.length == 7) {
                    ap += 4;
                    number = *(s32 *) (ap - 4);
                } else if (workspace.info.length == 8) {
                    ap += 4;
                    number = *(u32 *) (ap - 4);
                } else {
                    ap += 4;
                    number = *(unsigned int *) (ap - 4);
                }
                if (workspace.info.length == 2) {
                    number = (u16) number;
                }
                if (workspace.info.length == 1) {
                    number = (u8) number;
                }
                if (workspace.info.length == 4 || workspace.info.length == 6) {
                    text = Large(wide, workspace.buffer + 512, &workspace.info);
                    if (!text) {
                        goto invalid;
                    }
                } else {
                    text = Small(number, workspace.buffer + 512, &workspace.info);
                    if (!text) {
                        goto invalid;
                    }
                }
                length = workspace.buffer + 511 - text;
                break;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G': {
                if (workspace.info.length == 9) {
                    ap += 8;
                    value = *(long double *) (ap - 8);
                } else {
                    ap += 8;
                    value = *(double *) (ap - 8);
                }
                text = float2str(value, workspace.buffer + 512, workspace.info);
                if (!text) {
                    goto invalid;
                }
                length = workspace.buffer + 511 - text;
                break;
            }
            case 'a':
            case 'A': {
                if (workspace.info.length == 9) {
                    ap += 8;
                    value = *(long double *) (ap - 8);
                } else {
                    ap += 8;
                    value = *(double *) (ap - 8);
                }
                text = double2hex(value, workspace.buffer + 512, workspace.info);
                if (!text) {
                    goto invalid;
                }
                length = workspace.buffer + 511 - text;
                break;
            }
            case 's':
                if (workspace.info.length == 5) {
                    const u16 *value;
                    ap += 4;
                    value = *(const u16 **) (ap - 4);
                    if (!value) {
                        value = (const u16 *) data_021018c8;
                    }
                    if ((s32) wcstombs(workspace.buffer, value, 512) < 0) {
                        goto invalid;
                    }
                    text = workspace.buffer;
                } else {
                    ap += 4;
                    text = *(char **) (ap - 4);
                }
                if (!text) {
                    text = (char *) data_021018cc;
                }
                if (workspace.info.alternate) {
                    length = (u8) *text++;
                    if (workspace.info.precisionSet && length > workspace.info.precision) {
                        length = workspace.info.precision;
                    }
                } else if (workspace.info.precisionSet) {
                    const char *end;
                    length = workspace.info.precision;
                    end    = memchr(text, 0, length);
                    if (end) {
                        length = end - text;
                    }
                } else {
                    length = func_020e0db0(text);
                }
                break;
            case 'n': {
                ap += 4;
                text = *(char **) (ap - 4);
                switch (workspace.info.length) {
                    case 0:
                        *(int *) text = count;
                        break;
                    case 2:
                        *(s16 *) text = count;
                        break;
                    case 3:
                        *(long *) text = count;
                        break;
                    case 6:
                        *(s64 *) text = count;
                        break;
                    case 7:
                        *(int *) text = count;
                        break;
                    case 8:
                        *(int *) text = count;
                        break;
                    case 4:
                        *(s64 *) text = count;
                        break;
                }
                continue;
            }
            case 'c':
                ap += 4;
                workspace.buffer[0] = *(int *) (ap - 4);
                text                = workspace.buffer;
                length              = 1;
                break;
            case '%':
                workspace.buffer[0] = '%';
                text                = workspace.buffer;
                length              = 1;
                break;
            case 255:
                goto invalid;
            default:
            invalid: {
                int remainder = func_020e0db0(percent);
                if (remainder && !write(context, percent, remainder)) {
                    return -1;
                }
                return count + remainder;
            }
        }
        {
            field = length;
            if (workspace.info.justify != 0) {
                padding = workspace.info.justify == 2 ? '0' : ' ';
                if ((*text == '+' || *text == '-' || *text == ' ') && padding == '0') {
                    if (!write(context, text, 1)) {
                        return -1;
                    }
                    text++;
                    length--;
                }
                while (field < workspace.info.width) {
                    if (!write(context, &padding, 1)) {
                        return -1;
                    }
                    field++;
                }
            }
            if (length && !write(context, text, length)) {
                return -1;
            }
            if (workspace.info.justify == 0) {
                while (field < workspace.info.width) {
                    space = ' ';
                    if (!write(context, &space, 1)) {
                        return -1;
                    }
                    field++;
                }
            }
            count += field;
        }
    } while (*format);
done:
    return count;
}
