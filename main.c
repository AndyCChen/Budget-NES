#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "./includes/bus.h"
#include "./includes/cpu.h"
#include "./includes/cartridge.h"
#include "./includes/log.h"

bool nestest_log_flag = true;

int main(int argc, char *argv[])
{
   (void) argc;
   (void) argv;

   if ( !cartridge_load( argv[1] ) ) 
   {
      return EXIT_FAILURE;
   }

   nestest_log_open();

   cpu_reset();
   cpu_fetch_decode_execute();

   nestest_log_close();

   return 0;
}