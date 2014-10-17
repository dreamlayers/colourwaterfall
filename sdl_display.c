/* Winamp-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <stdbool.h>
#include <SDL.h>

#define width 640
#define height 480

static void sdlError(const char *str)
{
    fprintf(stderr, "Error %s\n", str);
    fprintf(stderr,"(reason for error: %s)\n", SDL_GetError());
    //exit(1);
}

static SDL_Surface *surface;

bool display_init(void) {
    SDL_Event event;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        sdlError("initializing SDL");
        return false;
    }

    //SDL_WM_SetCaption("Colour picker", "Colour picker");

    surface = SDL_SetVideoMode(width, height, 24, 0);

    if (!surface) {
        sdlError("setting video mode");
        return false;
    }
    return true;
}

bool display_render(unsigned char r, unsigned char g, unsigned char b) {
    static SDL_Rect scroll_src = { 0, 0, width, height - 1 };
    static SDL_Rect scroll_dest = { 0, 1, width, height - 1 };
    int i;
    unsigned char *p;

    SDL_BlitSurface(surface, &scroll_src, surface, &scroll_dest);
    SDL_LockSurface(surface);
    p = (unsigned char *)(surface->pixels);
    for (i = 0; i < width; i++) {
        *(p++) = b;
        *(p++) = g;
        *(p++) = r;
    }
    SDL_UnlockSurface(surface);
    SDL_UpdateRect(surface, 0, 0, width, height);
    return true;
}

unsigned int display_width(void)
{
    return width;
}

bool display_pollquit(void) {
    SDL_Event event;
    return SDL_PollEvent(&event) != 0 && event.type == SDL_QUIT;
}

void display_quit(void) {
    SDL_Quit();
}