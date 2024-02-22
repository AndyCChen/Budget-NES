#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "glad.h"
#include "SDL.h"

#include "bus.h"
#include "cpu.h"
#include "cartridge.h"
#include "log.h"
#include "display.h"

bool nestest_log_flag = true;

int main(int argc, char *argv[])
{
   (void) argc;
   (void) argv;

   if ( !display_init() )
   {
      return EXIT_FAILURE;
   }

   if ( !cartridge_load( argv[1] ) ) 
   {
      return EXIT_FAILURE;
   }

   graphics_create_shaders();
   graphics_create_triangle();

   bool done = false;
   while (!done)
   {
      display_process_event(&done);

      display_create_gui();

      display_render();
      display_update();
   }

   

   nestest_log_open();

   cpu_reset();

   for(int i = 0; i < 6375; ++i)
   {
      cpu_emulate_instruction();
   }
 
   nestest_log_close();
   
   display_shutdown();

   return 0;
}