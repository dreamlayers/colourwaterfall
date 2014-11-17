/* Winamp-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "vis.h"
#include "rgbm.h"
#define MODULETYPE winampVisModule

/***** Winamp module functions *****/

/* const not present due to Winamp API */
static char title[] = "Colour Waterfall";

static void config(struct MODULETYPE *this_mod) {
    MessageBox(this_mod->hwndParent,
               "This module is Copyright 2014, Boris Gjenero\n",
               title, MB_OK);
}

static int init(struct MODULETYPE *this_mod) {
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
}

static int render(struct winampVisModule *this_mod) {
    int noerror;
    noerror = rgbm_render(this_mod->spectrumData[0], 
                          this_mod->spectrumData[1]);
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
