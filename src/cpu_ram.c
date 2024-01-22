#include <stdint.h>

#include "../includes/cpu_ram.h"

static uint8_t cpu_ram[CPU_RAM_SIZE];

uint8_t cpu_ram_read(uint16_t position)
{
   return cpu_ram[position & 0x7FF];
}

void cpu_ram_write(uint16_t position, uint8_t data)
{
   cpu_ram[position & 0x7FF] = data;
}