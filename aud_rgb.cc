/* Audacious-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <glib.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include "rgbm.h"

class RGBWaterfall : public VisPlugin
{
public:
    static constexpr PluginInfo info = {
        N_("RGB Waterfall"),
        N_("standalone")
    };

    constexpr RGBWaterfall () : VisPlugin (info, Visualizer::MultiPCM) {}

    bool init ();
    void cleanup ();

    void render_multi_pcm (const float * pcm, int channels);
    void clear ();
};


__attribute__((visibility("default"))) RGBWaterfall aud_plugin_instance;

static double *left_samp = NULL, *right_samp = NULL;;

bool RGBWaterfall::init(void)
{
    bool res;

    res = rgbm_init();
    if (res) rgbm_get_wave_buffers(&left_samp, &right_samp);
    return res;
}

void RGBWaterfall::cleanup(void)
{
    rgbm_shutdown();
}

void RGBWaterfall::render_multi_pcm(const float * pcm, int channels)
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

void RGBWaterfall::clear(void)
{
}
