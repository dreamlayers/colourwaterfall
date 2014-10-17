/* Winamp-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

bool display_init(void);
bool display_render(int r, int g, int b);
bool display_pollquit(void);
void display_quit(void);