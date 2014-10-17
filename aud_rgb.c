/* Audacious-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <glib.h>
#include <audacious/plugin.h>
#include "rgbm.h"

static bool_t
rgblamp_init(void)
{
    return rgbm_init();
}

static void
rgblamp_cleanup(void)
{
    rgbm_shutdown();
}

static void
rgblamp_render_freq(const gfloat *freq)
{
    rgbm_render(freq);
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
    .render_freq = rgblamp_render_freq,
)

#if HAVE_VISIBILITY
#pragma GCC visibility pop
#endif
