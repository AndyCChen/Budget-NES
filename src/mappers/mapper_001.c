#include "mapper_001.h"
#include "mirror_config.h"

#include <string.h>

cartridge_access_mode_t mapper001_cpu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   Registers_001* mapper = (Registers_001*) internal_registers;
   cartridge_access_mode_t mode;

   // reading program rom
   if (position >= 0x8000)
   {
      mode = ACCESS_PRG_ROM;

		// SOROM, SUROM and SXROM use upper address lines of CHR select which 256kb of prg rom to use.
		// Increases prg rom capcity from 256kb to 512kb.
		uint32_t prg_256_offset = 0;
		if (mapper->PRG_256_bank_select)
		{
			prg_256_offset = 0x40000;
		}

      // fix last bank at $C000 and switch 16 KB bank at $8000
      if ((mapper->control & 0xC) == 0xC)
      {
         // reading bank at 0x8000
         if (position >= 0x8000 && position <= 0xBFFF)
         {
            *mapped_addr = prg_256_offset + (position & 0x3FFF) + ((mapper->PRG_bank & 0xF) * 0x4000);
         }
         // reading bank at 0xC000
         else
         {
				if (header->prg_rom_size > 16)
				{
					if (prg_256_offset)
						*mapped_addr = (position & 0x3FFF) + ((header->prg_rom_size - 1) * 0x4000);
					else
						*mapped_addr = (position & 0x3FFF) + (0xF * 0x4000);
				}
				else
				{
	            *mapped_addr = prg_256_offset + (position & 0x3FFF) + ((header->prg_rom_size - 1) * 0x4000);
				}
         }
      }
      // fix first bank at $8000 and switch 16 KB bank at $C000
      else if ((mapper->control & 0xC) == 0x8)
      {
         // reading bank at 0x8000
         if (position >= 0x8000 && position <= 0xBFFF)
         {
				*mapped_addr = prg_256_offset + (position & 0x3FFF);
         }
         // reading bank at 0xC000
         else
         {
            *mapped_addr = prg_256_offset + (position & 0x3FFF) + ((mapper->PRG_bank & 0xF) * 0x4000);
         }
      }
      // switch 32 KB at $8000, ignoring low bit of bank number
      else
      {
         *mapped_addr = prg_256_offset + (position & 0x7FFF) + ( ((mapper->PRG_bank >> 1) & 0x3) * 0x8000) ;
      }

   }
   // reading program ram
   else if (position >= 0x6000 && position <= 0x7FFF && !(mapper->PRG_bank & 0x10))
   {
      mode = ACCESS_PRG_RAM;
		*mapped_addr = (position & 0x1FFF) + (mapper->PRG_ram_bank * 0x2000);
   }
   else
   {
      mode = NO_CARTRIDGE_DEVICE;
   }

   return mode;
}

cartridge_access_mode_t mapper001_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, size_t *mapped_addr, void* internal_registers)
{
   (void) header;
   Registers_001* mapper = (Registers_001*) internal_registers;
   cartridge_access_mode_t mode;

   // writes into program rom is intercepted and used to configure bank switching
   if (position >= 0x8000)
   {
      mode = NO_CARTRIDGE_DEVICE;

      // bit 7 set means reset shift register and set prg bank mode to 3
      if (data & 0x80)
      {
         mapper->shift_register = 0x10;
         mapper->control |= 0xC;
      }
      // bit 0 being set means shift register is full
      else if (mapper->shift_register & 0x1)
      {
         uint8_t dest = ( (data & 0x1) << 4 ) | ( (mapper->shift_register >> 1) & 0xF );
         mapper->shift_register = 0x10;

         // chr bank 0
         if (position >= 0xA000 && position <= 0xBFFF)
         {
				// https://www.nesdev.org/wiki/MMC1#SOROM,_SUROM_and_SXROM
				// handle special rom variants
				if (header->prg_rom_size > 16)
				{
					mapper->CHR_bank_0 = dest & 0x1;
					mapper->PRG_ram_bank = (dest & 0xC) >> 2;
					mapper->PRG_256_bank_select = (dest & 0x10) >> 4;
				}
				else
				{
					mapper->CHR_bank_0 = dest;
				}
         }
         // chr bank 1
         else if (position >= 0xC000 && position <= 0xDFFF)
         {
				if (header->prg_rom_size > 16)
				{
					mapper->CHR_bank_1 = dest & 0x1;
					mapper->PRG_ram_bank = (dest & 0xC) >> 2;
					mapper->PRG_256_bank_select = (dest & 0x10) >> 4;
				}
				else
				{
					mapper->CHR_bank_1 = dest;
				}
         }
         // prg bank
         else if (position >= 0xE000)
         {
            mapper->PRG_bank = dest;
         }
         // control register
         else
         {
            mapper->control = dest;
         }
      }
      // else just shift bit 0 of data being written into the msb of shift register
      // and shift the register right 1 bit
      else
      {
         mapper->shift_register = (mapper->shift_register >> 1) | ( (data & 0x1) << 4 );
      }
   }
   // writing to program ram if program ram enabled
   else if (position >= 0x6000 && position <= 0x7FFF && !(mapper->PRG_bank & 0x10))
   {
      mode = ACCESS_PRG_RAM;
		*mapped_addr = (position & 0x1FFF) + (mapper->PRG_ram_bank * 0x2000);
   }
   // else prg ram disabled
   else 
   {
      mode = NO_CARTRIDGE_DEVICE;
   }

   return mode;
}

cartridge_access_mode_t mapper001_ppu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   (void) header;
   Registers_001* mapper = (Registers_001*) internal_registers;
   cartridge_access_mode_t mode;

   // reading chr memory
   if ( position <= 0x1FFF )
   {
      mode = ACCESS_CHR_MEM;

      // switching two 4kb banks
      if (mapper->control & 0x10)
      {
         // reading bank at 0x0000
         if (position <= 0xFFF)
         {
            *mapped_addr = (position & 0xFFF) + ((mapper->CHR_bank_0 & 0x1F) * 0x1000);
         }
         // reading bank at 0x1000
         else
         {
            *mapped_addr = (position & 0xFFF) + ((mapper->CHR_bank_1 & 0x1F) * 0x1000);
         }
      }
      // switch one 8kb bank
      else
      {
			// SOROM, SUROM, SXROM only use 8kb of chr rom/ram, no bank switching
			if (header->prg_rom_size > 16)
			{
				*mapped_addr = (position & 0x1FFF);
			}
			else
			{
			  *mapped_addr = (position & 0x1FFF) + ( ((mapper->CHR_bank_0 & 0x1E) >> 1) * 0x2000 );
			}
      }
   }
   // reading ppu nametable vram
   else
   {
      mode = ACCESS_VRAM;
      position = position & 0x2FFF; // addresses 0x3000 - 0x3FFF are mirrors of 0x2000 - 0x2FFF

      // vertical mirroring
      if ((mapper->control & 0x3) == 2)
      {
         mirror_config_vertical(&position);
      }
      // horizontal mirroring
      else if ((mapper->control & 0x3) == 3)
      {
         mirror_config_horizontal(&position);
      }
      // one screen mirroring for lower bank
      else if ((mapper->control & 0x3) == 0)
      {
        // position &= 0x3FF;
         mirror_config_single_screen(&position, 0);
         
      }
      // one screen mirroring for upper bank
      else
      {
         mirror_config_single_screen(&position, 1);
      }

      *mapped_addr = position & 0x7FF;
   }

   return mode;
}

cartridge_access_mode_t mapper001_ppu_write(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   Registers_001* mapper = (Registers_001*) internal_registers;
   cartridge_access_mode_t mode;

   // writing to chr-memory
   if (position <= 0x1FFF)
   {
      // only allow writes if chr memory is ram and not rom
      if (header->chr_rom_size == 0)
      {
         mode = ACCESS_CHR_MEM;

         // switching two 4kb banks
         if (mapper->control & 0x10)
         {
            // reading bank at 0x0000
            if (position <= 0xFFF)
            {
               *mapped_addr = (position & 0xFFF) + ((mapper->CHR_bank_0 & 0x1F) * 0x1000);
            }
            // reading bank at 0x1000
            else
            {
               *mapped_addr = (position & 0xFFF) + ((mapper->CHR_bank_1 & 0x1F) * 0x1000);
            }
         }
         // switch one 8kb bank
         else
         {
				// SOROM, SUROM, SXROM only use 8kb of chr rom/ram, no bank switching
				if (header->prg_rom_size > 16)
				{
					*mapped_addr = (position & 0x1FFF);
				}
				else
				{
					*mapped_addr = (position & 0x1FFF) + (((mapper->CHR_bank_0 & 0x1E) >> 1) * 0x2000);
				}
         }
      }
      else
      {
         mode = NO_CARTRIDGE_DEVICE;
      }
   }
   // writting to ppu nametable vram
   else
   {
      mode = ACCESS_VRAM;
      position = position & 0x2FFF; // addresses 0x3000 - 0x3FFF are mirrors of 0x2000 - 0x2FFF

      // vertical mirroring
      if ((mapper->control & 0x3) == 2)
      {
         mirror_config_vertical(&position);
      }
      // horizontal mirroring
      else if ((mapper->control & 0x3) == 3)
      {
         mirror_config_horizontal(&position);
      }
      // one screen mirroring for lower bank
      else if ((mapper->control & 0x3) == 0)
      {
         mirror_config_single_screen(&position, 0);
      }
      // one screen mirroring for upper bank
      else
      {
         mirror_config_single_screen(&position, 1);
      }

      *mapped_addr = position & 0x7FF;
   }

   return mode;
}

void mapper001_init(nes_header_t* header, void* internal_registers)
{
   Registers_001* mapper = (Registers_001*) internal_registers;
	memset(mapper, 0, sizeof(Registers_001));

	mapper->control = 0xC | (header->nametable_arrangement ? 0x2 : 0x3); // initalize mirroring configuration
	mapper->shift_register = 0x10;
}
