#include <stdlib.h>
#include <stdio.h>

#include "mapper.h"
#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_004.h"
#include "mapper_007.h"
#include "mapper_009.h"

// mappers that don't generate irqs use this default function
static bool irq_signaled_default(void* internal_registers)
{
	(void)internal_registers;
	return false;
}

bool load_mapper(uint32_t mapper_id, mapper_t *mapper, void** mapper_registers)
{
   bool status = true;

   switch (mapper_id)
   {
      case 0:
      {
         mapper->cpu_read     = &mapper000_cpu_read;
         mapper->cpu_write    = &mapper000_cpu_write;
         mapper->ppu_read     = &mapper000_ppu_read;
         mapper->ppu_write    = &mapper000_ppu_write;
         mapper->init         = &mapper000_init;
			mapper->irq_signaled = &irq_signaled_default;
         *mapper_registers = NULL;
         break;
      }
      case 1:
      {  
         mapper->cpu_read     = &mapper001_cpu_read;
         mapper->cpu_write    = &mapper001_cpu_write;
         mapper->ppu_read     = &mapper001_ppu_read;
         mapper->ppu_write    = &mapper001_ppu_write;
         mapper->init         = &mapper001_init;
			mapper->irq_signaled = &irq_signaled_default;
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
         mapper->cpu_read     = &mapper002_cpu_read;
         mapper->cpu_write    = &mapper002_cpu_write;
         mapper->ppu_read     = &mapper002_ppu_read;
         mapper->ppu_write    = &mapper002_ppu_write;
         mapper->init         = &mapper002_init;
			mapper->irq_signaled = &irq_signaled_default;
         *mapper_registers    = malloc(sizeof(Registers_002));

         if (mapper_registers == NULL)
         {
            printf("Mapper 002 register malloc failed!\n");
            status = false;
         }

         break;
      }
		case 4:
		{
			mapper->cpu_read     = &mapper004_cpu_read;
			mapper->cpu_write    = &mapper004_cpu_write;
			mapper->ppu_read     = &mapper004_ppu_read;
			mapper->ppu_write    = &mapper004_ppu_write;
			mapper->init         = &mapper004_init;
			mapper->irq_signaled = &mapper004_irq_signaled;
			*mapper_registers    = malloc(sizeof(Registers_004));

			if (mapper_registers == NULL)
			{
				printf("Mapper 004 register malloc failed!\n");
				status = false;
			}

			break;
		}
		case 7:
		{
			mapper->cpu_read     = &mapper007_cpu_read;
			mapper->cpu_write    = &mapper007_cpu_write;
			mapper->ppu_read     = &mapper007_ppu_read;
			mapper->ppu_write    = &mapper007_ppu_write;
			mapper->init         = &mapper007_init;
			mapper->irq_signaled = &irq_signaled_default;
			*mapper_registers    = malloc(sizeof(Registers_007));

			if (mapper_registers == NULL)
			{
				printf("Mapper 007 register malloc failed!\n");
				status = false;
			}

			break;
		}
		case 9:
		{
			mapper->cpu_read     = &mapper009_cpu_read;
			mapper->cpu_write    = &mapper009_cpu_write;
			mapper->ppu_read     = &mapper009_ppu_read;
			mapper->ppu_write    = &mapper009_ppu_write;
			mapper->init         = &mapper009_init;
			mapper->irq_signaled = &irq_signaled_default;
			*mapper_registers    = malloc(sizeof(Registers_009));

			if (mapper_registers == NULL)
			{
				printf("Mapper 009 register malloc failed!\n");
				status = false;
			}

			break;
		}

      default:
         status = false;
   }

   return status;
}
