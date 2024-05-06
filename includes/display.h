#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include <stdint.h>
#include <vec4.h>

/**
 * display.h handles setting up and shutting down all sdl
 * and opengl utilities, creating all imGui elements, and
 * renders/updates to the screen
*/

bool display_init(void);
void display_render(void);
void display_shutdown(void);
void display_process_event(bool* done);
void set_pixel_color(uint32_t row, uint32_t col, vec3 color);

#endif
