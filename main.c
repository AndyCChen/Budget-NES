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

   create_shaders();
   create_triangle();

   bool done = false;
   while (!done)
   {
      display_process_event(&done);

      display_create_gui();

      display_render();
      display_update();
   }

    if ( !cartridge_load( argv[1] ) ) 
   {
      return EXIT_FAILURE;
   }

   nestest_log_open();

   cpu_reset();
   cpu_fetch_decode_execute();

   nestest_log_close();
   
   display_shutdown();

   return 0;
}