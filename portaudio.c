#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <portaudio.h>
#include <pthread.h>
#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#endif
#include "rgbm.h"

#define inputsize (RGBM_NUMSAMP * 2 / 3)

/* Default sound device, for visualizing playback from other programs */
#ifdef WIN32
#define MONITOR_NAME "Stereo Mix"
#else
#define MONITOR_NAME "pulse_monitor"
#endif

static PaStream *stream = NULL;
static double *left_samp = NULL, *right_samp = NULL;
/* Each ring contains enough samples for one output block. Input block
 * size must be smaller or equal to output block size. Input samples 
 * are added at ring_write, wrapping around the rings. They are copied
 * from there to the output buffers, forming un-wrapped output blocks.
 */
static double left_ring[RGBM_NUMSAMP], right_ring[RGBM_NUMSAMP];
static int ring_write = 0;
pthread_mutex_t mutex;
pthread_cond_t cond;
static bool sound_available = false;

static void error(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
    exit(-1);
}

static void sound_store(const int16_t *input) {
    int i, outidx;

    outidx = ring_write;
    for (i = 0; i < inputsize; i++) {
        left_ring[outidx] = input[i * 2] / 32768.0;
        right_ring[outidx] = input[i * 2 + 1] / 32768.0;
        outidx++;
        if (outidx >= RGBM_NUMSAMP) outidx = 0;
    }
    ring_write = outidx;
}

static int pa_callback(const void *input, void *output,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags, void *userData) {
    if (frameCount != inputsize) {
        error("callback got unexpected number of samples.");
    }

    pthread_mutex_lock(&mutex);
    sound_store((int16_t *)input);
    sound_available = true;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return paContinue;
}

static void sound_open(const char *devname) {
    PaError err;
    PaStreamParameters inputParameters;
    PaDeviceIndex numDevices;
#ifdef WIN32
    unsigned int namelen;
#endif
#ifdef __linux__
    int oldstderr;

    /* Send useless ALSA stderr output to /dev/null */
    oldstderr = dup(2);
    if (oldstderr >= 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, 2);
            close(devnull);
        } else {
            close(oldstderr);
            oldstderr = -1;
        }
    }
#endif
    err = Pa_Initialize();
#ifdef __linux__
    if (oldstderr >= 0) {
        /* dup2 will also close 2 before replacing it */
        dup2(oldstderr, 2);
        close(oldstderr);
    }
#endif
    if(err != paNoError) error("initializing PortAudio.");

    if ((devname == NULL) ?
#ifdef WIN32
        0
#else
        /* PULSE_SOURCE sets default input device. Don't automatically
           over-ride that with our monitor name guess. */
        (getenv("PULSE_SOURCE") != NULL)
#endif
        /* Use "default" to explicitly select default sound source.
           Needed because otherwise MONITOR_NAME would be used. */
        : (!strcmp(devname, "default"))) {
        inputParameters.device = Pa_GetDefaultInputDevice();
    } else {
        if (devname == NULL) devname = MONITOR_NAME;

        /* Search through devices to find one matching devname */
        numDevices = Pa_GetDeviceCount();
#ifdef WIN32
        namelen = strlen(devname);
#endif
        for (inputParameters.device = 0; inputParameters.device < numDevices;
             inputParameters.device++) {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
            if (deviceInfo != NULL && deviceInfo->name != NULL &&
#ifdef WIN32
                /* Allow substring matches, for eg. "Stereo Mix (Sound card name)" */
                !strncmp(deviceInfo->name, devname, namelen)
#else
                !strcmp(deviceInfo->name, devname)
#endif
                ) break;
        }
        if (inputParameters.device == numDevices) {
            fprintf(stderr, "Warning: couldn't find %s sound device, using default\n",
                    devname);
            inputParameters.device = Pa_GetDefaultInputDevice();
        }
    }

    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = paInt16; /* PortAudio uses CPU endianness. */
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(&stream,
                      &inputParameters,
                      NULL, //&outputParameters,
                      44100,
                      inputsize,
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
    pthread_mutex_lock(&mutex);

    while (!sound_available) {
        pthread_cond_wait(&cond, &mutex);
    }

    memcpy(&left_samp[0], &left_ring[ring_write],
           sizeof(double) * (RGBM_NUMSAMP - ring_write));
    memcpy(&right_samp[0], &right_ring[ring_write],
           sizeof(double) * (RGBM_NUMSAMP - ring_write));
    if (ring_write > 0) {
        /* Samples are wrapped around the ring. Copy the second part. */
        memcpy(&left_samp[RGBM_NUMSAMP - ring_write], &left_ring[0],
               sizeof(double) * ring_write);
        memcpy(&right_samp[RGBM_NUMSAMP - ring_write], &right_ring[0],
               sizeof(double) * ring_write);
    }

    sound_available = false;
    pthread_mutex_unlock(&mutex);
}

static void sound_visualize(void) {
    do {
        sound_retrieve();
    } while (rgbm_render_wave());
}

int main(int argc, char **argv) {
    char *snddev = NULL;

    if (argc == 2) {
        snddev = argv[1];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [sound device]\n", argv[0]);
        return -1;
    }

    if (!rgbm_init()) {
        error("initializing visualization");
    }
    rgbm_get_wave_buffers(&left_samp, &right_samp);

    sound_open(snddev);

    sound_visualize();

    sound_close();
    rgbm_shutdown();

    return 0;
}
