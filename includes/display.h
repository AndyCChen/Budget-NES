#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include <stdint.h>
#include <vec4.h>
#include <SDL.h>

/**
 * display.h handles setting up and shutting down all sdl
 * and opengl utilities, creating all imGui elements, and
 * renders/updates to the screen
*/

// enum for setting the emulator to be running normally or paused for single stepping through instructions or a frame
typedef enum Emulator_Run_State_t
{
   EMULATOR_PAUSED,
   EMULATOR_RUNNING,
   EMULATOR_UNLOADED, // waiting for user to load rom file
} Emulator_Run_State_t;

typedef struct Emulator_State_t 
{
   /**
    * bit 0: fullscreen
    * bit 1: screen size 1x
    * bit 2: screen size 2x
    * bit 3: screen size 3x
   */
   uint8_t display_size;
   bool is_cpu_debug;              // toggle cpu debug widget
   bool is_cpu_intr_log;           // toggle instruction logging
   bool is_pattern_table_open;     // toggle pattern table viewer
   Emulator_Run_State_t run_state;
   bool was_paused;                // was emulator paused on previous frame
   bool is_instruction_step;       // true: steps the emulator forward by 1 instruction, false: do nothing
} Emulator_State_t;

bool display_init(void);
void display_clear(void);
void display_render(void);
void display_update(void);
void display_shutdown(void);
void display_update_color_buffer(void);
void display_process_event(bool* done);
void set_viewport_pixel_color(uint32_t row, uint32_t col, vec3 color);
Emulator_State_t* get_emulator_state(void);
SDL_Window* display_get_window(void);

/**
 * Check if window was recently moved.
 * On windows 10 moving a window blocks the execution of the program
 * so we need to adjust delta times for the emulation speed and ingnore accumulated
 * time during this period.
 */
bool display_is_window_moved(void);

#endif
