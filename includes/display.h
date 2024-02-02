#ifndef GUI_H
#define GUI_H

#include <stdbool.h>

/**
 * display.h handles setting up and shutting down all sdl
 * and opengl utilities, creating all imGui elements, and
 * renders/updates to the screen
*/

/**
 * setup sdl, create window, and opengl context
 * set ImGui io config flags
 * \returns true on success and false on failure
*/
bool display_init();

/**
 * render imgui components
*/
void display_render();

/**
 * updates the screen
*/
void display_update();

/**
 * call functions to shutdown and free all resources
*/
void display_shutdown();

/**
 * process sdl events
*/
void display_process_event(bool* done);

/**
 * create imgui components
*/
void display_create_gui();

void create_triangle();
void create_shaders();

#endif