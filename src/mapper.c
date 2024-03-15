#include "../includes/mapper.h"
#include "../includes/mappers/mapper_000.h"

bool load_mapper(uint32_t mapper_id, mapper_t *mapper)
{
   bool status;

   switch (mapper_id)
   {
      case 0:
      {
         mapper->cpu_read = &mapper000_cpu_read;
         mapper->ppu_read = &mapper000_ppu_read;
         mapper->cpu_write = &mapper000_cpu_write;
         mapper->ppu_write = &mapper000_ppu_write;
         status = true;
         break;
      }
      default:
         status = false;
   }

   return status;
}