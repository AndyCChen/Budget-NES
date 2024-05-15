#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include <stdint.h>

typedef enum JOYPAD_BUTTONS
{
   BUTTON_A =      1 << 0,
   BUTTON_B =      1 << 1,
   BUTTON_SELECT = 1 << 2,
   BUTTON_START =  1 << 3,
   BUTTON_UP =     1 << 4,
   BUTTON_DOWN =   1 << 5,
   BUTTON_LEFT =   1 << 6,
   BUTTON_RIGHT =  1 << 7,
} JOYPAD_BUTTONS;

/**
 * Set the value of the strobe bit in the controller to signal continuous reloading of input shift registers.
*/
void controller_write_strobe(uint8_t data);

/**
 * Returns state of a button from shift register of controller 1 then shifts left by 1 bit.
*/
uint8_t controller1_read(void);

/**
 * Returns state of a button from shift register of controller 2 then shifts left by 1 bit.
*/
uint8_t controller2_read(void);

/**
 * Reloads controller shift registers with input states polled by sdl during each frame
 * if the strobe bit is 1, else if it is 0 then the shift registers are not reloaded.
*/
void controller_reload_shift_registers(void);

/**
 * Set the corresponding button state when pressed.
*/
void controller1_set_button_down(JOYPAD_BUTTONS button);

/**
 * Clear the corresponding button state when released.
*/
void controller1_set_button_up(JOYPAD_BUTTONS button);

#endif
