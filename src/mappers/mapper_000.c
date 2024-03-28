#include "../includes/mappers/mapper_000.h"
#include "../includes/bus.h"

// address ranges used by cpu to access cartridge space

#define CPU_CARTRIDGE_PRG_RAM_START 0x6000
#define CPU_CARTRIDGE_PRG_RAM_END   0x7FFF
#define CPU_CARTRIDGE_PRG_ROM_START 0x8000

// address ranges used by ppu to access cartridge space

#define PPU_CARTRIDGE_PATTERN_TABLE_END 0x1FFF

cartridge_access_mode_t mapper000_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr)
{
   cartridge_access_mode_t mode;

   if ( position >= CPU_CARTRIDGE_PRG_ROM_START )
   {
      *mapped_addr = (header->prg_rom_size > 1) ? position & 0x7FFF : position & 0x3FFF ; // rom size for mapper 0 is 16kb or 32kb
      mode = ACCESS_PRG_ROM;
   }
   else if ( position >= CPU_CARTRIDGE_PRG_RAM_START && position <= CPU_CARTRIDGE_PRG_RAM_END )
   {
      *mapped_addr = position & 0x1FFF;
      mode = ACCESS_PRG_RAM;
   }
   else
   {
      mode = NO_CARTRIDGE_DEVICE;
   }

   return mode;
}

cartridge_access_mode_t mapper000_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr)
{
   cartridge_access_mode_t mode;

   if ( position <= PPU_CARTRIDGE_PATTERN_TABLE_END )
   {
      *mapped_addr = position;
      mode = ACCESS_CHR_MEM;
   }
   else
   {
      mode = NO_CARTRIDGE_DEVICE;
   }

   return mode;
}

cartridge_access_mode_t mapper000_cpu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr)
{
   cartridge_access_mode_t mode;

   if ( position >= CPU_CARTRIDGE_PRG_RAM_START && position <= CPU_CARTRIDGE_PRG_RAM_END )
   {
      *mapped_addr = position & 0x1FFF;
      mode = ACCESS_PRG_RAM;
   }
   else
   {
      mode = NO_CARTRIDGE_DEVICE;
   }

   return mode;
}

cartridge_access_mode_t mapper000_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr)
{
   cartridge_access_mode_t mode;

   if ( position <= PPU_CARTRIDGE_PATTERN_TABLE_END )
   {
      if ( header->chr_rom_size == 0 ) // 0 size chr-rom means chr-ram is used instead so writes are allowed
      {
         *mapped_addr = position;
         mode = ACCESS_CHR_MEM;
      }
   }
   else
   {
      mode = NO_CARTRIDGE_DEVICE;
   }

   return mode;
}