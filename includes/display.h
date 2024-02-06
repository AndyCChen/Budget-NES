#ifndef GUI_H
#define GUI_H

#include <stdbool.h>

/**
 * display.h handles setting up and shutting down all sdl
 * and opengl utilities, creating all imGui elements, and
 * renders/updates to the screen
*/

bool display_init();
void display_render();
void display_update();
void display_shutdown();
void display_process_event(bool* done);
void display_create_gui();

void graphics_create_triangle();
void graphics_create_shaders();

#endif