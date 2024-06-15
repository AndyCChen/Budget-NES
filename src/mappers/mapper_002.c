#include "../includes/mappers/mapper_002.h"
#include "../includes/mappers/mirror_config.h"

cartridge_access_mode_t mapper002_cpu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   Registers_002* mapper = (Registers_002*) internal_registers;
   cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

   // reading program rom
   if (position >= 0x8000)
   {
      mode = ACCESS_PRG_ROM;

      // reading first bank at 0x8000 - 0xBFFF which is switchable by the mapper
      if (position <= 0xBFFF)
      {
         *mapped_addr = (mapper->PRG_bank * 0x4000) + (position & 0x3FFF);
      }
      // reading second bank at 0xC000 - 0xFFFF which is fixed to the last bank of program rom
      else
      {
         *mapped_addr = ( (header->prg_rom_size - 1) * 0x4000 ) + (position & 0x3FFF);
      }

      *mapped_addr &= (header->prg_rom_size * 0x4000) - 1;
   }
   
   return mode;
}

cartridge_access_mode_t mapper002_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, size_t *mapped_addr, void* internal_registers)
{
   (void) mapped_addr;
   (void) header;
   Registers_002* mapper = (Registers_002*) internal_registers;
   cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

   // configuring prg bank select in mapper
   if (position >= 0x8000)
   {
      mapper->PRG_bank = data;
   }

   return mode;
}

cartridge_access_mode_t mapper002_ppu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   (void) internal_registers;
   cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

   // reading from chr memory (pattern tables)
   if (position <= 0x1FFF)
   {
      mode = ACCESS_CHR_MEM;
      *mapped_addr = position;
   }
   // reading from vram (nametables)
   else
   {
      position &= 0x2FFF;

      if ( header->nametable_arrangement )
      {
         mirror_config_vertical(&position);
      }
      else
      {
         mirror_config_horizontal(&position);
      }

      *mapped_addr = position & 0x7FF;
      mode = ACCESS_VRAM;
   }

   return mode;
}

cartridge_access_mode_t mapper002_ppu_write(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   (void) internal_registers;
   cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

   // writing to chr memory (pattern tables)
   if ( position <= 0x1FFF )
   {  
      if ( header->chr_rom_size == 0 ) // 0 size chr-rom means chr-ram is used instead so writes are allowed
      {
         *mapped_addr = position;
         mode = ACCESS_CHR_MEM;
      }
   }
   // writing to vram (nametables)
   else
   {
      position &= 0x2FFF;

      if ( header->nametable_arrangement )
      {
         mirror_config_vertical(&position);
      }
      else
      {
         mirror_config_horizontal(&position);
      }

      *mapped_addr = position & 0x7FF;
      mode = ACCESS_VRAM;
   }

   return mode;
}

void mapper002_init(nes_header_t* header, void* internal_registers)
{
   (void) header;
   Registers_002* mapper = (Registers_002*) internal_registers;
   mapper->PRG_bank = 0;
}


