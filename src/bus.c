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

// read single byte from bus and clocks cpu by 1 tick
uint8_t cpu_bus_read(uint16_t position)
{
   static uint8_t data = 0;
   cpu_tick();

   // addressing cartridge space
   if ( position >= CPU_CARTRIDGE_START )
   {
      data = cartridge_cpu_read(position);
   }  
   // accessing 2 kb cpu ram address space
   else if ( position <= CPU_RAM_END )
   {
      data = cpu_ram[position & 0x7FF];
   }
   // accessing ppu registers
   else if ( position >= CPU_PPU_REG_START && position <= CPU_PPU_REG_END )
   {
      
      data = ppu_port_read( 0x2000 | (position & 0x7) );
   }
   
   return data;
}

// write single byte to bus and clocks cpu by 1 tick
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
   // writing to cartridge (i.e. program RAM, NOT ROM)
   else if ( position >= 0x6000 && position <= 0x7FFF )
   {
      cartridge_cpu_write(position, data);
   }
   else if (position == 0x4041)
   {
      printf("oamdma\n");
   }
}

/**
 * Used by disassembler to read from memory without affecting the NES during
 * runtime. Does not read from ppu ports in order to avoid
 * disrupting the PPU during rendering.
*/
uint8_t DEBUG_cpu_bus_read(uint16_t position)
{
   static uint8_t data = 0;

   // addressing cartridge space
   if ( position >= CPU_CARTRIDGE_START )
   {
      data = cartridge_cpu_read(position);
   }  
   // accessing 2 kb cpu ram address space
   else if ( position <= CPU_RAM_END )
   {
      data = cpu_ram[position & 0x7FF];
   }
   
   return data;
}
