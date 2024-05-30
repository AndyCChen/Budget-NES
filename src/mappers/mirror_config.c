#include "../../includes/mappers/mirror_config.h"

void mirror_config_vertical(uint16_t* position)
{
   *position = *position & ~(0x0800);
}

void mirror_config_horizontal(uint16_t* position)
{
   if (*position >= 0x2000 && *position <= 0x27FF)
   {
      *position = *position & ~(0x0400);
   }
   else
   {
      *position = ( *position & ~(0x0C00) ) | 0x400;
   }
}
