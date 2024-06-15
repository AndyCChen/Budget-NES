#include <stdlib.h>
#include <stdio.h>

#include "../includes/mapper.h"
#include "../includes/mappers/mapper_000.h"
#include "../includes/mappers/mapper_001.h"
#include "../includes/mappers/mapper_002.h"

bool load_mapper(uint32_t mapper_id, mapper_t *mapper, void** mapper_registers)
{
   bool status = true;

   switch (mapper_id)
   {
      case 0:
      {
         mapper->cpu_read  = &mapper000_cpu_read;
         mapper->cpu_write = &mapper000_cpu_write;
         mapper->ppu_read  = &mapper000_ppu_read;
         mapper->ppu_write = &mapper000_ppu_write;
         mapper->init      = &mapper000_init;
         *mapper_registers = NULL;
         break;
      }
      case 1:
      {  
         mapper->cpu_read  = &mapper001_cpu_read;
         mapper->cpu_write = &mapper001_cpu_write;
         mapper->ppu_read  = &mapper001_ppu_read;
         mapper->ppu_write = &mapper001_ppu_write;
         mapper->init      = &mapper001_init;
         *mapper_registers = malloc(sizeof(Registers_001));

         if (mapper_registers == NULL)
         {
            printf("Mapper 001 register malloc failed!\n");
            status = false;
         }

         break;
      }
      case 2:
      {
         mapper->cpu_read  = &mapper002_cpu_read;
         mapper->cpu_write = &mapper002_cpu_write;
         mapper->ppu_read  = &mapper002_ppu_read;
         mapper->ppu_write = &mapper002_ppu_write;
         mapper->init      = &mapper002_init;
         *mapper_registers = malloc(sizeof(Registers_002));

         if (mapper_registers == NULL)
         {
            printf("Mapper 002 register malloc failed!\n");
            status = false;
         }

         break;
      }
      default:
         status = false;
   }

   return status;
}
