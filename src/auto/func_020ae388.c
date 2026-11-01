/* Reconstructed from USA ARM9 at 0x020ae388 (272 bytes).
 * WM callback that func_020ae2f8 passes to func_020d78c0 (wireless helper code,
 * strings "WM_StartDataSharing OK\n" and "unknown indicate, state = %d\n").
 * A nonzero error code at +2 is recorded by func_020ad8b4 and the system state
 * (data_0210b320 +0x20, set by func_020ad7a4) goes to ERROR. For indication 10
 * (+4): in mode 2 (+0x44), from CONNECTED it calls func_020ae498 and goes to
 * ERROR if that returns 0, and from KEYSHARING it returns at once. In mode 4 it
 * starts data sharing on data_0210b720 (port 13, aid bitmap 0xf, 0x7c bytes,
 * double mode from the byte at +0) and goes to DATASHARING, or to ERROR if the
 * start fails. Any other mode goes to CONNECTED. Indications 11..13 do nothing
 * and other values are printed. Paths that do not return early store 45 at +0x48.
 */
#include "m2c.h"
extern void func_020ad528(char *a, ...);
extern void func_020ad8b4(s32);
extern void func_020ad7a4(s32);
extern s32 func_020ae498(void);
extern s32 WM_StartDataSharing(void *, s32, s32, s32, s32);
extern char data_0210014c[];
extern char data_0210016c[];
extern u8 data_0210b720[];
extern u8 data_0210b320[];

/* System states, as named by the func_020ad7a4 print table data_020ffd20. */
enum {
    WH_SYSSTATE_CONNECTED   = 4,
    WH_SYSSTATE_DATASHARING = 5,
    WH_SYSSTATE_KEYSHARING  = 6,
    WH_SYSSTATE_ERROR       = 9
};
/* WM_StartDataSharing port and dataLength arguments. */
enum {
    DS_PORT        = 0xd,
    DS_DATA_LENGTH = 0x7c
};

void func_020ae388(void *arg0) {
    u16 err = M2C_FIELD(arg0, u16 *, 2);
    if (err != 0) {
        func_020ad8b4(err);
        func_020ad7a4(WH_SYSSTATE_ERROR);
        return;
    }
    switch (M2C_FIELD(arg0, u16 *, 4) - 10) {
        case 0:
            if (M2C_FIELD(&data_0210b320, s32 *, 0x44) == 2) {
                if (M2C_FIELD(&data_0210b320, s32 *, 0x20) == WH_SYSSTATE_CONNECTED) {
                    if (func_020ae498() != 0) {
                        return;
                    }
                    func_020ad7a4(WH_SYSSTATE_ERROR);
                    return;
                }
                if (M2C_FIELD(&data_0210b320, s32 *, 0x20) == WH_SYSSTATE_KEYSHARING) {
                    return;
                }
            } else if (M2C_FIELD(&data_0210b320, s32 *, 0x44) == 4) {
                s32 result = WM_StartDataSharing(
                    data_0210b720, DS_PORT,
                    (u16) (&data_0210b720[0xf] -
                           data_0210b720), // FAKE: 0xf folded only in the backend, so aidBitmap (r2) is set before port (r1)
                    DS_DATA_LENGTH, M2C_FIELD(&data_0210b320, u8 *, 0));
                if (result != 0) {
                    func_020ad8b4(result);
                    func_020ad7a4(WH_SYSSTATE_ERROR);
                    return;
                }
                func_020ad528(data_0210016c);
                func_020ad7a4(WH_SYSSTATE_DATASHARING);
            } else {
                func_020ad7a4(WH_SYSSTATE_CONNECTED);
            }
            break;
        case 1:
        case 2:
        case 3:
            break;
        default:
            func_020ad528(data_0210014c, *((u16 *) arg0 + 2));
            break;
    }
    M2C_FIELD(&data_0210b320, s32 *, 0x48) = 0x2D;
}
