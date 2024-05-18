#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "glad_loader/include/glad/glad.h"
#include "SDL.h"

#include "includes/cpu.h"
#include "includes/ppu.h"
#include "includes/cartridge.h"
#include "includes/log.h"
#include "includes/display.h"

static bool budgetNES_init(const char* rom_path);
static void budgetNES_shutdown(void);

int main(int argc, char *argv[])
{
   (void) argc;
   (void) argv;

   if ( !budgetNES_init( argv[1] ) )
   {
      return EXIT_FAILURE;
   }

   Emulator_State_t* emulator_state = get_emulator_state();

   bool done = false;
   while (!done)
   {
      display_clear();
      display_process_event(&done); 

      switch (emulator_state->run_state)
      {
         case EMULATOR_RUNNING:
         {
            for (int i = 0; i < 2000; ++i) 
            {
               cpu_emulate_instruction();
            }
            break;
         }
         case EMULATOR_PAUSED:
         {
            if (emulator_state->instruction_step)
            {
               cpu_emulate_instruction();
               emulator_state->instruction_step = false;
            }

            break;
         }
      }

      display_render(); 
      display_update();
   }

   budgetNES_shutdown();

   return 0;
}

static bool budgetNES_init(const char* rom_path)
{
   if ( !display_init() || !cartridge_load( rom_path ) || !ppu_load_palettes("./ntscpalette.pal") )
   {
      return false;
   }

   cpu_init();
   return true;
}

static void budgetNES_shutdown(void)
{
   cartridge_free_memory();
   display_shutdown();
}
