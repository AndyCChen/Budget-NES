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

// enum for setting the emulator to be running normally or paused for single stepping through instructions or a frame
typedef enum Emulator_Run_State_t
{
   EMULATOR_PAUSED = 0,
   EMULATOR_RUNNING = 1,
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
   bool cpu_debug;                 // toggle cpu debug widget
   bool pattern_table_viewer;  // toggle pattern table viewer
   Emulator_Run_State_t run_state; 
   bool instruction_step;          // true: steps the emulator forward by 1 instruction, false: do nothing
   uint8_t refresh_rate;
} Emulator_State_t;

bool display_init(void);
void display_clear(void);
void display_render(void);
void display_update(void);
void display_shutdown(void);
uint8_t display_get_refresh_rate(void);
void display_process_event(bool* done);
void set_pixel_color(uint32_t row, uint32_t col, vec3 color);
Emulator_State_t* get_emulator_state(void);

#endif
