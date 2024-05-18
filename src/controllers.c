#include "../includes/controllers.h"

// emulator joypad variables track button states and are updated when sdl polls for inputs every frame

static uint8_t emulator_joypad1 = 0;

static uint8_t emulator_joypad2 = 0;

// shift registers hold each of the 8 button states and is shifted 1 bit right when it is read

static uint8_t joypad1_shift = 0;
static uint8_t joypad2_shift = 0;
static uint8_t strobe; // 1: controller will continuously reload the shift registers, 0: controller will stop reloading shift registers

void controller_write_strobe(uint8_t data)
{
   strobe = data;
}

uint8_t controller1_read(void)
{
   uint8_t button_state = joypad1_shift & 0x1;
   joypad1_shift = joypad1_shift >> 1;
   joypad1_shift |= 0x80;
   
   return button_state;
}

uint8_t controller2_read(void) // only 1 controller supported for now!
{
   uint8_t button_state = joypad2_shift & 0x1;
   joypad2_shift = joypad2_shift >> 1;
   joypad2_shift |= 0x80;

   return button_state;
}

void controller_reload_shift_registers(void)
{
   if (strobe & 0x1)
   {
      joypad1_shift = emulator_joypad1;
      joypad2_shift = emulator_joypad2;
   }
}

void controller1_set_button_down(JOYPAD_BUTTONS button) // only 1 controller supported for now!
{
   switch (button)
   {
      case BUTTON_UP:
      {
         emulator_joypad1 |= BUTTON_UP;
         break;
      }
      case BUTTON_DOWN:
      {
         emulator_joypad1 |= BUTTON_DOWN;
         break;
      }
      case BUTTON_LEFT:
      {
         emulator_joypad1 |= BUTTON_LEFT;
         break;
      }
      case BUTTON_RIGHT:
      {
         emulator_joypad1 |= BUTTON_RIGHT;
         break;
      }
      case BUTTON_A:
      {
         emulator_joypad1 |= BUTTON_A;
         break;
      }
      case BUTTON_B:
      {
         emulator_joypad1 |= BUTTON_B;
         break;
      }
      case BUTTON_START:
      {
         emulator_joypad1 |= BUTTON_START;
         break;
      }
      case BUTTON_SELECT:
      {
         emulator_joypad1 |= BUTTON_SELECT;
         break;
      }  
   }
}

void controller1_set_button_up(JOYPAD_BUTTONS button) // only 1 controller supported for now!
{
   switch (button)
   {
      case BUTTON_UP:
      {
         emulator_joypad1 &= ~BUTTON_UP;
         break;
      }
      case BUTTON_DOWN:
      {
         emulator_joypad1 &= ~BUTTON_DOWN;
         break;
      }
      case BUTTON_LEFT:
      {
         emulator_joypad1 &= ~ BUTTON_LEFT;
         break;
      }
      case BUTTON_RIGHT:
      {
         emulator_joypad1 &= ~ BUTTON_RIGHT;
         break;
      }
      case BUTTON_A:
      {
         emulator_joypad1 &= ~ BUTTON_A;
         break;
      }
      case BUTTON_B:
      {
         emulator_joypad1 &= ~ BUTTON_B;
         break;
      }
      case BUTTON_START:
      {
         emulator_joypad1 &= ~ BUTTON_START;
         break;
      }
      case BUTTON_SELECT:
      {
         emulator_joypad1 &= ~ BUTTON_SELECT;
         break;
      }  
   }
}
