#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

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

   if ( !display_gui_init() )
   {
      return EXIT_FAILURE;
   }

   bool done = false;
   while (!done)
   {
      display_process_event(&done);

      display_gui();
      display_gui_render();
      display_update();
   }

   /* if ( !cartridge_load( argv[1] ) ) 
   {
      return EXIT_FAILURE;
   }

   nestest_log_open();

   cpu_reset();
   cpu_fetch_decode_execute();

   nestest_log_close(); */
   
   display_gui_shutdown();

   return 0;
}