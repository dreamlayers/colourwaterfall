/* Shared visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

/* Define to output averaged per-bin sums for calibration */
/* #define RGBM_LOGGING */

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "rgbm.h"
#include "display.h"
#ifdef RGBM_LOGGING
#include <stdio.h>
#endif

#if defined(RGBM_AUDACIOUS)

#define RGBPORT "/dev/ttyUSB0"
/* Calibrated in Audacious 3.4 in Ubuntu 13.10 */
/* Frequency of bin is (i+1)*44100/512 (array starts with i=0).
 * Value corresponds to amplitude (not power or dB).
 */

/* Moving average size when value is increasing */
#define RGBM_AVGUP 5
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 15

/* Colour scaling to balance red and blue with green */
#define RGBM_REDSCALE 2.40719835270744
#define RGBM_BLUESCALE 1.60948950058501

/* Overall scaling to adapt output to values needed by librgblamp */
#define RGBM_SCALE (12000.0)
#define RGBM_LIMIT 4095

/* Table for bin weights for summing bin powers (amplitued squared) to green.
 * Below RGBM_PIVOTBIN, red + green = 1.0. At and above, red + blue = 1.0.
 */
#include "greentab_audacious.h"

/* Table for adjusting bin amplitudes using equal loudness contour. */
#define HAVE_FREQ_ADJ
static const double freq_adj[RGBM_USEBINS] = {
#include "freqadj_audacious.h"
};

#elif defined(RGBM_WINAMP)

#define RGBPORT "COM8"

/* Calibrated in Winamp v5.666 Build 3512 (x86)
 * Frequency of bin roughly corresponds to (i - 1) * 44100 / 1024
 * This doesn't behave like a good FFT. Increased amplitude causes
 * peak to broaden more than it grows. I don't know the mathematical
 * connection between the signal and bins.
 */

/* Moving average size when value is increasing */
#define RGBM_AVGUP 7
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 20

/* Colour scaling to balance red and blue with green.
 * Calibrated using pink noise, after setting above parameters.
 */
#define RGBM_REDSCALE 2.25818534475793
#define RGBM_BLUESCALE 1.337960735802697

/* Overall scaling to adapt output to values needed by librgblamp */
#define RGBM_SCALE (3.5)
#define RGBM_LIMIT 4095

#include "greentab_winamp.h"

#else
#error Need to set define for type of music player.
#endif

/*
 * Static global variables
 */

static double binavg[3];
/* Set after successful PWM write, and enables rgb_matchpwm afterwards */
static int wrotepwm;

#ifdef RGBM_LOGGING
static double testsum[RGBM_USEBINS];
static unsigned int testctr;
FILE *testlog = NULL;
static double avgavg[3];
#define RGBM_TESTAVGSIZE 1000
#endif

/*
 * Internal routines
 */

static void sum_to_stripe(const RGBM_BINTYPE left_bins[RGBM_NUMBINS],
                          const RGBM_BINTYPE right_bins[RGBM_NUMBINS],
                          double **stripe, unsigned int width){
    int rgb, i = 0, limit = RGBM_PIVOTBIN, lr;
    const RGBM_BINTYPE *left_right[2] = { left_bins, right_bins };

    /* First, sum other to red before pivot.
     * Then sum other to blue from pivot to end
     */
    for (rgb = 0; rgb < 3; rgb += 2) {
        for (; i < limit; i++) {
            double green[2], other[2];
            int pos;

            for (lr = 0; lr <= 1; lr++) {
                double bin = left_right[lr][i];
#ifdef HAVE_FREQ_ADJ
                bin *= freq_adj[i];
#endif
                bin *= bin;
                green[lr] = green_tab[i] * bin;
                other[lr] = bin - green[lr];
            }

            pos = left_right[0][i] + left_right[1][i];
            if (pos > 0) {
                pos = width * left_right[1][i] / pos;
            }
            stripe[rgb][pos] += other[0] + other[1];
            stripe[1][pos] += green[0] + green[1];
        }
        limit = RGBM_USEBINS;
    }
} /* rgbm_sumbins */

static void rgbm_avgsums(const double sums[3],
                         double avg[3],
                         double scale,
                         double bound) {
    int i = 0;
    for (i = 0; i < 3; i++) {
        double avgsize, sum;

        sum = (double)sums[i] * scale;

        if (sum > avg[i]) {
            avgsize = RGBM_AVGUP;
        } else {
            avgsize = RGBM_AVGDN;
        }

        avg[i] = ((avgsize - 1.0) * avg[i] + sum) / avgsize;
        if (avg[i] > bound) avg[i] = bound;
    } /* for i */
} /* rgbm_avgsums */

#ifdef RGBM_LOGGING
static void rgbm_testsum(const RGBM_BINTYPE bins[RGBM_NUMBINS]) {
    int i;

    if (testlog == NULL) return;

    for (i = 0; i < RGBM_USEBINS; i++) {
        double bin = bins[i];
#ifdef HAVE_FREQ_ADJ
        bin *= freq_adj[i];
#endif
        testsum[i] = (testsum[i] * (RGBM_TESTAVGSIZE - 1) + bin)
                     / RGBM_TESTAVGSIZE;
    }

    if (testctr++ >= RGBM_TESTAVGSIZE) {
        testctr = 0;
        for (i = 0; i < RGBM_USEBINS; i++) {
            fprintf(testlog, "%i:%f\n", i, testsum[i]);
        }
    }
} /* RGBM_LOGGING */
#endif /* RGBM_LOGGING */

/*
 * Interface routines
 */

int rgbm_init(void) {
    int i;

    if (!display_init())
        return false;

    for (i = 0; i < 3; i++) binavg[i] = 0.0;
#ifdef RGBM_LOGGING
    for (i = 0; i < RGBM_USEBINS; i++) testsum[i] = 0.0;
    testlog = fopen("rgbm.log", "w");
    testctr = 0;
#endif
    wrotepwm = 0;
    return true;
}

void rgbm_shutdown(void) {
    display_quit();
#ifdef RGBM_LOGGING
    if (testlog != NULL) fclose(testlog);
#endif
}

static int rgb_pwm2srgb(double n) {
    double r = n / 4095.0;
    int t;
    if (r <= 0.0031308) {
        r = 12.92 * r;
    } else {
        r = 1.055 * pow(r, 1/2.4) - 0.055;
    }
    t = r * 255.0 + 0.5;
    if (t > 255) t = 255;
    else if (t < 0) t = 0;
    return t;
}

static void zero_stripe(double **stripe, unsigned int width) {
    int i, j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < width; j++) {
            stripe[i][j] = 0;
        }
    }
}

static double **get_stripe(unsigned int width) {
    static unsigned int cur_width = 0;
    static double *cur_alloc = NULL;
    static double *rgb_ptrs[3] = { NULL, NULL, NULL };

    if (width != cur_width) {
        if (cur_alloc != NULL) free(cur_alloc);

        if (width == 0) return NULL;

        cur_alloc = malloc(width * 3 * sizeof(double));
        if (cur_alloc == NULL) return NULL;
        cur_width = width;

        rgb_ptrs[0] = &cur_alloc[0];
        rgb_ptrs[1] = &cur_alloc[width];
        rgb_ptrs[2] = &cur_alloc[width * 2];
    }

    return rgb_ptrs;
}

int rgbm_render(const RGBM_BINTYPE left_bins[RGBM_NUMBINS],
                const RGBM_BINTYPE right_bins[RGBM_NUMBINS]) {
    //int res;

    int width;
    double **stripe;

    width = display_width();
    stripe = get_stripe(width);
    if (stripe == NULL) return 0;

    zero_stripe(stripe, width);
    sum_to_stripe(left_bins, right_bins, stripe, width);

#if 0
    rgbm_sumbins(bins, sums);
    sums[0] *= RGBM_REDSCALE;
    sums[2] *= RGBM_BLUESCALE;
    rgbm_avgsums(sums, binavg, RGBM_SCALE, RGBM_LIMIT);
#endif
#ifdef RGBM_LOGGING
    for (res = 0; res < 3; res++) {
        avgavg[res] = (avgavg[res] * (RGBM_TESTAVGSIZE-1) + binavg[res])
                      / RGBM_TESTAVGSIZE;
    }
    if (testlog != NULL) {
        if (testctr == RGBM_TESTAVGSIZE-1) {
            fprintf(testlog, "%9f; %9f; %9f\n", avgavg[0], avgavg[1], avgavg[2]);
        }
        rgbm_testsum(bins);
        fprintf(testlog, "%9f, %9f, %9f\n", binavg[0], binavg[1], binavg[2]);
    }
#endif
    //display_render((int)binavg[0] >> 4 , (int)binavg[1] >> 4, (int)binavg[2] >> 4);
    display_render(stripe[0], stripe[1], stripe[2]);
    return !display_pollquit();
//    res = rgb_pwm(binavg[0], binavg[1], binavg[2]);
 //   return res;
} /* rgbm_render */
