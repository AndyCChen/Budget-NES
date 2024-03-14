#include "../includes/mappers/mapper_000.h"

void mapper000_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode_t mode)
{
   switch (mode)
   {
      case PROGRAM_ROM:
      {
         *mapped_addr = (header->program_rom_size > 1) ? position & 0x7FFF : position & 0x3FFF;
         break;
      }
      case PROGRAM_RAM:
      {
         break;
      }
   }   
}

void mapper000_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode_t mode)
{
   switch (position)
   {
      case CHR_ROM:
      {
         *mapped_addr = position;
         break;
      }
   }
}

void mapper000_cpu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode_t mode)
{
   // if chr_rom_size is 0 then chr_ram is being used so writes are allowed
   if (header->chr_rom_size == 0)
   {

   }
}

void mapper000_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, cartridge_mode_t mode)
{
   // if chr_rom_size is 0 then chr_ram is being used so writes are allowed
   if (header->chr_rom_size == 0)
   {

   }
}