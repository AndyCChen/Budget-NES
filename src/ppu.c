#include "../includes/ppu.h"

static uint8_t ppu_ram[PPU_RAM_SIZE];

void ppu_cpu_write(uint8_t position, uint8_t data)
{
   ppu_ram[position] = data;
}

uint8_t ppu_cpu_read(uint8_t position)
{
   return ppu_ram[position];
}