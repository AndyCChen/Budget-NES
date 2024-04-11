#ifndef GUI_H
#define GUI_H

#include <stdbool.h>

/**
 * display.h handles setting up and shutting down all sdl
 * and opengl utilities, creating all imGui elements, and
 * renders/updates to the screen
*/

bool display_init(void);
void display_render(void);
void display_shutdown(void);
void display_process_event(bool* done);

void graphics_create_pixels(void);
void graphics_create_shaders(void);

#endif