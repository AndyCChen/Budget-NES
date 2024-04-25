#include <stdint.h>
#include <stdio.h>

#include "../includes/bus.h"
#include "../includes/cartridge.h"
#include "../includes/ppu.h"
#include "../includes/cpu.h"

// address ranges used by cpu to access cartridge space

#define CPU_CARTRIDGE_START 0x4020

// address ranges used by cpu to access ppu registers

#define CPU_PPU_REG_START 0x2000
#define CPU_PPU_REG_END   0x3FFF

#define CPU_RAM_SIZE 1024 * 2
#define CPU_RAM_END  0x1FFF

static uint8_t cpu_ram[CPU_RAM_SIZE];

// read single byte from bus
uint8_t cpu_bus_read(uint16_t position)
{
   cpu_tick();
   return cpu_bus_read_no_tick(position);
}

// write single byte to bus
void cpu_bus_write(uint16_t position, uint8_t data)
{
   cpu_tick();
   // accessing 2 kb cpu ram address space
   if ( position <= CPU_RAM_END )
   {
      cpu_ram[position & 0x7FF] = data;
   }
   // accessing ppu registers
   else if ( position >= CPU_PPU_REG_START && position <= CPU_PPU_REG_END )
   {
      ppu_port_write( 0x2000 | (position & 0x7), data );
   }
}

/**
 * Reads byte from bus without clocking the cpu
*/
uint8_t cpu_bus_read_no_tick(uint16_t position)
{
   // addressing cartridge space
   if ( position >= CPU_CARTRIDGE_START )
   {
      return cartridge_cpu_read(position);
   }  
   // accessing 2 kb cpu ram address space
   else if ( position <= CPU_RAM_END )
   {
      return cpu_ram[position & 0x7FF];
   }
   // accessing ppu registers
   else if ( position >= CPU_PPU_REG_START && position <= CPU_PPU_REG_END )
   {
      return ppu_port_read( 0x2000 | (position & 0x7) );
   }

   return 0x00;
}