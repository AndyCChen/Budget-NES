#ifndef GUI_H
#define GUI_H

#include <stdbool.h>

/**
 * setup sdl, create window, and opengl context
 * set ImGui io config flags
 * \returns true on success and false on failure
*/
bool display_gui_init();

/**
 * render imgui components
*/
void display_gui_render();

/**
 * updates the screen
*/
void display_update();

/**
 * call functions to shutdown and free all resources
*/
void display_gui_shutdown();

/**
 * process sdl events
*/
void display_process_event(bool* done);

/**
 * create imgui components
*/
void display_gui();

#endif