/* Header file for RGB lamp visualization plugin. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#ifndef _RGBM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RGBM_AUDACIOUS)

/* Number of FFT bins */
#define RGBM_NUMSAMP 512
#define RGBM_NUMBINS 256
/* Type of FFT bins */
#define RGBM_BINTYPE double
#define RGBM_SAMPTYPE double

#elif defined (RGBM_WINAMP)

#define RGBM_NUMBINS 576
#define RGBM_BINTYPE unsigned char

#else
#error Need to set define for type of music player.
#endif

/* Here int really means bool, but some compilers can't handle bool */
int rgbm_init(void);
void rgbm_shutdown(void);
int rgbm_render(const RGBM_BINTYPE left_bins[RGBM_NUMBINS],
                const RGBM_BINTYPE right_bins[RGBM_NUMBINS]);
void rgbm_get_wave_buffers(double *left[], double *right[]);
int rgbm_render_wave(void);

#ifdef __cplusplus
}
#endif

#endif /* !_RGBM_H_ */
