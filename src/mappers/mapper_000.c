#include "../includes/mappers/mapper_000.h"

#define CPU_CARTRIDGE_PRG_RAM_START 0x6000
#define CPU_CARTRIDGE_PRG_RAM_END   0x7FFF
#define CPU_CARTRIDGE_PRG_ROM_START 0x8000

#define PPU_CARTRIDGE_PATTERN_TABLE_END 0x1FFF
#define PPU_VRAM_START 0x2000
#define PPU_VRAM_END 0x3EFF

#define NAMETABLE_0_START 0x2000
#define NAMETABLE_0_END   0x23BF
#define NAMETABLE_1_START 0x2400
#define NAMETABLE_1_END   0x27FF
#define NAMETABLE_2_START 0x2800
#define NAMETABLE_2_END   0x2BFF
#define NAMETABLE_3_START 0x2C00
#define NAMETABLE_3_END   0x2FFF


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
   position = position & 0x3FFF; // ppu addresses only a 14 bit address space

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
         // vertical mirroring
         if ( position >= NAMETABLE_2_START && position <= NAMETABLE_2_END )
         {
            position = position & NAMETABLE_0_END;
         }
         else if ( position >= NAMETABLE_3_START && position <= NAMETABLE_3_END )
         {
            position = position & NAMETABLE_1_END;
         }
      }
      else
      {
         // horizontal mirroring
         if ( position >= NAMETABLE_1_START && position <= NAMETABLE_1_END )
         {
            position = position & NAMETABLE_0_END;
         }
         else if ( position >= NAMETABLE_3_START && position <= NAMETABLE_3_END )
         {
            position = position & NAMETABLE_2_END;
         }
      }

      *mapped_addr = position & 0x7FF;
      mode = ACCESS_VRAM;
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
   position = position & 0x3FFF; // ppu addresses only a 14 bit address space

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
      position = position & 0x2FFF; // addresses 0x3000 - 0x3FFF are mirrors of 0x2000 - 0x2FFF

      if ( header->nametable_arrangement )
      {
         // vertical mirroring
         if ( position >= NAMETABLE_2_START && position <= NAMETABLE_2_END )
         {
            position = position & NAMETABLE_0_END;
         }
         else if ( position >= NAMETABLE_3_START && position <= NAMETABLE_3_END )
         {
            position = position & NAMETABLE_1_END;
         }
      }
      else
      {
         // horizontal mirroring
         if ( position >= NAMETABLE_1_START && position <= NAMETABLE_1_END )
         {
            position = position & NAMETABLE_0_END;
         }
         else if ( position >= NAMETABLE_3_START && position <= NAMETABLE_3_END )
         {
            position = position & NAMETABLE_2_END;
         }
      }

      *mapped_addr = position & 0x7FF;
      mode = ACCESS_VRAM;
   }

   return mode;
}