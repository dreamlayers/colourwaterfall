/* Audacious-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <glib.h>
#include <audacious/plugin.h>
#include "rgbm.h"

static double *left_samp = NULL, *right_samp = NULL;;

static bool_t
rgblamp_init(void)
{
    bool_t res;

    res = rgbm_init();
    if (res) rgbm_get_wave_buffers(&left_samp, &right_samp);
    return res;
}

static void
rgblamp_cleanup(void)
{
    rgbm_shutdown();
}

#if 0
static void
rgblamp_render_freq(const gfloat *freq)
{
    rgbm_render(freq, freq);
}
#endif

static void rgblamp_render_multi_pcm(const float * pcm, int channels)
{
    int i;
    if (channels == 1) {
        for (i = 0; i < RGBM_NUMSAMP; i++) {
            left_samp[i] = pcm[i];
            right_samp[i] = pcm[i];
        }
    } else if (channels > 1) {
        for (i = 0; i < RGBM_NUMSAMP; i++) {
            left_samp[i] = pcm[i * channels];
            right_samp[i] = pcm[i * channels + 1];
        }
    } else {
        return;
    }

    rgbm_render_wave();
}


#if defined(__GNUC__) && __GNUC__ >= 4
#define HAVE_VISIBILITY 1
#else
#define HAVE_VISIBILITY 0
#endif

#if HAVE_VISIBILITY
#pragma GCC visibility push(default)
#endif

AUD_VIS_PLUGIN(
    .name = "RGB Lamp",
    .init = rgblamp_init,
    .cleanup = rgblamp_cleanup,
    //.about = aosd_about,
    //.configure = rgblamp_configure,
    //.render_freq = rgblamp_render_freq,
    .render_multi_pcm = rgblamp_render_multi_pcm,
)

#if HAVE_VISIBILITY
#pragma GCC visibility pop
#endif
