/* Petits outils communs des mods de base, sans bibliotheque C (les mods Windows 32 bits n'en ont pas).
 * SPDX-License-Identifier: MIT */
#ifndef NDS_UTIL_H
#define NDS_UTIL_H

#include "nds_mod.h"

#define NDS_PI 3.14159265f

static inline nds_s32 nds_fx(float v) { return (nds_s32)(v * 4096.0f); }
static inline float nds_real(nds_s32 v) { return (float)v / 4096.0f; }

static inline float nds_wrap_angle(float a) {
    while (a > NDS_PI) a -= 2 * NDS_PI;
    while (a < -NDS_PI) a += 2 * NDS_PI;
    return a;
}

/* serie de Taylor, sur [-pi/2, pi/2] */
static inline float nds_sine(float a) {
    a = nds_wrap_angle(a);
    if (a > NDS_PI / 2) a = NDS_PI - a;
    else if (a < -NDS_PI / 2) a = -NDS_PI - a;
    float x2 = a * a;
    return a * (1 - x2 / 6 * (1 - x2 / 20 * (1 - x2 / 42 * (1 - x2 / 72))));
}

static inline float nds_cosine(float a) { return nds_sine(a + NDS_PI / 2); }

/* assez juste pour un cap de depart : dichotomie sur le sinus et le cosinus */
static inline float nds_arc_tangent(float y, float x) {
    float low = -NDS_PI, high = NDS_PI;
    for (int i = 0; i < 24; i++) {
        float mid = (low + high) / 2;
        if (nds_cosine(mid) * y - nds_sine(mid) * x > 0) low = mid;
        else high = mid;
    }
    return (low + high) / 2;
}

/* la valeur d'un reglage en nombre (une touche se garde par son code HID) */
static inline int nds_setting_int(const nds_api *api, nds_mod *mod, const char *key, int fallback) {
    const char *s = api->setting(mod, key);
    if (!s || *s < '0' || *s > '9') return fallback;
    int v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + *s++ - '0';
    return v;
}

static inline int nds_lang_fr(const nds_api *api) { return api->language && api->language[0] == 'f'; }

#if defined(_WIN32) && !defined(NDS_NO_ENTRY)
/* sans bibliotheque C : le point d'entree de la DLL ne fait rien. Un mod de plusieurs sources definit NDS_NO_ENTRY dans toutes
 * sauf une (ou n'inclut nds_util.h qu'une fois). */
#if defined(__i386__)
int __stdcall DllMainCRTStartup(void *inst, nds_u32 reason, void *reserved) { return 1; }
#else
int DllMainCRTStartup(void *inst, nds_u32 reason, void *reserved) { return 1; }
#endif
#endif

#endif
