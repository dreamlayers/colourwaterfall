#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <portaudio.h>
#include <pthread.h>
#include "rgbm.h"

static PaStream *stream = NULL;
static double *left_samp = NULL, *right_samp = NULL;
int16_t pcm[RGBM_NUMSAMP * 2];
pthread_mutex_t mutex;
pthread_cond_t cond;
static bool sound_available = false;

static void error(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
    exit(-1);
}

static int pa_callback(const void *input, void *output,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags, void *userData) {
    if (frameCount != RGBM_NUMSAMP) {
        error("callback got unexpected number of samples.");
    }

    pthread_mutex_lock(&mutex);
    memcpy(pcm, input, sizeof(pcm));
    sound_available = true;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return paContinue;
}

static void sound_open(void) {
    PaError err;

    err = Pa_Initialize();
    if(err != paNoError) error("initializing PortAudio.");

    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paInt16; /* PortAudio uses CPU endianness. */
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(&stream,
                      &inputParameters,
                      NULL, //&outputParameters,
                      44100,
                      RGBM_NUMSAMP,
                      paClipOff,
                      pa_callback,
                      NULL); /* no callback userData */
    if(err != paNoError) error("opening PortAudio stream");

    err = Pa_StartStream( stream );
    if(err != paNoError) error("starting PortAudio stream");

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

static void sound_close(void) {
    PaError err;

    err = Pa_StopStream(stream);
    if (err != paNoError) error("stopping PortAudio stream");

    err = Pa_CloseStream(stream);
    if (err != paNoError) error("closing PortAudio stream");

    err = Pa_Terminate();
    if (err != paNoError) error("terminating PortAudio");

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

static void sound_retrieve(void) {
    int i;

    pthread_mutex_lock(&mutex);

    while (!sound_available) {
        pthread_cond_wait(&cond, &mutex);
    }

    for (i = 0; i < RGBM_NUMSAMP; i++) {
        left_samp[i] = pcm[i * 2] / 32768.0;
        right_samp[i] = pcm[i * 2 + 1] / 32768.0;
    }

    sound_available = false;
    pthread_mutex_unlock(&mutex);
}

static void sound_visualize(void) {
    do {
        sound_retrieve();
    } while (rgbm_render_wave());
}

int main(void) {
    if (!rgbm_init()) {
        error("initializing visualization");
    }
    rgbm_get_wave_buffers(&left_samp, &right_samp);

    sound_open();

    sound_visualize();

    sound_close();
    rgbm_shutdown();

    return 0;
}
