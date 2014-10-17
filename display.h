/* Winamp-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

bool display_init(void);
unsigned int display_width(void);
/* These must be arrays of size display_width() */
bool display_render(double *r, double *g, double *b);
bool display_pollquit(void);
void display_quit(void);