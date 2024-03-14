#include "../includes/ppu.h"

static uint8_t ppu_ram[PPU_RAM_SIZE];

void ppu_cpu_write(uint16_t position, uint8_t data)
{// todo
   switch(position)
   {// mask first 3 bits
      case PPU_CONTROL:
      case PPU_MASK:
      case PPU_STATUS:
      case OAM_ADDR:
      case OAM_DATA:
      case PPU_SCROLL:
      case PPU_ADDR:
      case PPU_DATA:
      case OAM_DMA:
         break;
   }
}

uint8_t ppu_cpu_read(uint16_t position)
{// todo
   switch(position)
   {
      case PPU_CONTROL:
      case PPU_MASK:
      case PPU_STATUS:
      case OAM_ADDR:
      case OAM_DATA:
      case PPU_SCROLL:
      case PPU_ADDR:
      case PPU_DATA:
      case OAM_DMA:
         break;
   }
   return 0;
}