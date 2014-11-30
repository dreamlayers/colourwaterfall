/* Winamp-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <windows.h>
#include "vis.h"
#include "rgbm.h"
#include "../gen_vishelper/vishelp.h"
#define MODULETYPE winampVisModule

/***** Winamp module functions *****/

/* const not present due to Winamp API */
static char title[] = "Colour Waterfall";

static void config(struct MODULETYPE *this_mod) {
    MessageBox(this_mod->hwndParent,
               "This module is Copyright 2014, Boris Gjenero\n",
               title, MB_OK);
}

#ifdef RGBM_FFT
visHelperAPI *vishelper_api = NULL;

static int find_helper(void) {
    HMODULE vishelper;
    visHelperAPIGetter api_getter;

    vishelper = GetModuleHandle("gen_vishelper.dll");
    if (vishelper == NULL) return 0;

    api_getter = (visHelperAPIGetter)GetProcAddress(vishelper,
                                                    "getVisHelperAPI");
    if (api_getter == NULL) return 0;
    vishelper_api = api_getter();
    if (vishelper_api == NULL || vishelper_api->version != 0) return 0;
    if (!vishelper_api->start()) return 0;

    return 1;
}
#endif

static int init(struct MODULETYPE *this_mod) {
#ifdef RGBM_FFT
    if (!find_helper()) {
        MessageBox(this_mod->hwndParent,
                   "Cannot find visualization helper", title,
                   MB_ICONSTOP | MB_OK);
        return 1;
    }
#endif

    if (rgbm_init()) {
        return 0;
    } else {
        MessageBox(this_mod->hwndParent,
                   "Cannot initialize visualization", title,
                   MB_ICONSTOP | MB_OK);
        return 1;
    }
}

static void quit(struct MODULETYPE *this_mod) {
    rgbm_shutdown();
#ifdef RGBM_FFT
    vishelper_api->stop();
#endif
}

#ifdef RGBM_FFT
static void copy_data(const int16_t *input) {
    int i;
    double *left, *right;
    rgbm_get_wave_buffers(&left, &right);

    for (i = 0; i < RGBM_NUMSAMP; i++) {
        left[i] = input[i * 2] / 32768.0;
        right[i] = input[i * 2 + 1] / 32768.0;
    }
}
#endif

static int render(struct winampVisModule *this_mod) {
    int noerror;
#ifdef RGBM_FFT
    int16_t *data = (int16_t *)vishelper_api->get_raw_data(RGBM_NUMSAMP * 4);
    if (data == NULL) return 1;
    copy_data(data);
    noerror = rgbm_render_wave();
#else
    noerror = rgbm_render(this_mod->spectrumData[0], 
                          this_mod->spectrumData[1]);
#endif
    return noerror ? 0 : 1;
}

/***** Winamp module interface *****/

// third module (VU meter)
static winampVisModule mod = {
    title,
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    25,     // latencyMS
    25,     // delayMS
    2,      // spectrumNch
    0,      // waveformNch
    { { 0, 0 }, }, // spectrumData
    { { 0, 0 }, }, // waveformData
    config,
    init,
    render,
    quit
};

static MODULETYPE *getModule(int which) {
    switch (which) {
        case 0:  return &mod;
        default: return NULL;
    }
}

// Module header, includes version, description, and address of the module retriever function
static winampVisHeader hdr = { VIS_HDRVER, title, getModule };

#ifdef __cplusplus
extern "C" {
#endif
// this is the only exported symbol. returns our main header.
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(void);
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(void) {
    return &hdr;
}
#ifdef __cplusplus
}
#endif
