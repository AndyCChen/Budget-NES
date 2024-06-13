#include "../includes/mappers/mapper_000.h"
#include "../includes/mappers/mirror_config.h"

#define CPU_CARTRIDGE_PRG_RAM_START 0x6000
#define CPU_CARTRIDGE_PRG_RAM_END   0x7FFF
#define CPU_CARTRIDGE_PRG_ROM_START 0x8000

#define PPU_CARTRIDGE_PATTERN_TABLE_END 0x1FFF

cartridge_access_mode_t mapper000_cpu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   (void) internal_registers;
   cartridge_access_mode_t mode;

   if ( position >= CPU_CARTRIDGE_PRG_ROM_START )
   {
      *mapped_addr = (header->prg_rom_size > 1) ? position & 0x7FFF : position & 0x3FFF ; // rom size for mapper 0 is 32kb or 16kb
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

cartridge_access_mode_t mapper000_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, size_t *mapped_addr, void* internal_registers)
{
   (void) data;
   (void) header;
   (void) internal_registers;
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

cartridge_access_mode_t mapper000_ppu_read(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   (void) internal_registers;
   cartridge_access_mode_t mode;

   if ( position <= PPU_CARTRIDGE_PATTERN_TABLE_END )
   {
      *mapped_addr = position;
      mode = ACCESS_CHR_MEM;
   }
   else
   {
      position = position & 0x2FFF; // addresses 0x3000 - 0x3FFF are mirrors of 0x2000 - 0x2FFF

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

cartridge_access_mode_t mapper000_ppu_write(nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers)
{
   (void) internal_registers;
   cartridge_access_mode_t mode;
   
   if ( position <= PPU_CARTRIDGE_PATTERN_TABLE_END )
   {  
      if ( header->chr_rom_size == 0 ) // 0 size chr-rom means chr-ram is used instead so writes are allowed
      {
         *mapped_addr = position;
         mode = ACCESS_CHR_MEM;
      }
      else
      {
         mode = NO_CARTRIDGE_DEVICE;
      }
   }
   else
   {
      position = position & 0x2FFF; // addresses 0x3000 - 0x3FFF are mirrors of 0x2000 - 0x2FFF

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

// mapper 0 has no internal registers so it's init function does nothing
void mapper000_init(nes_header_t* header, void* internal_registers)
{
   (void) internal_registers;
   (void) header;
}
