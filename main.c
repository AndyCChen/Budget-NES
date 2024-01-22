#include <stdio.h>
#include <stdlib.h>

#include "includes/bus.h"
#include "includes/cpu.h"
#include "includes/cartridge.h"

int main(int argc, char *argv[])
{
   (void) argc;
   (void) argv;

   if ( !cartridge_load( argv[1] ) ) return EXIT_FAILURE;

   cpu_reset();
   cpu_fetch_decode_execute();

   return 0;
}