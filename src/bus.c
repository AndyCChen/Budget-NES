#include <stdint.h>
#include <stdio.h>

#include "../includes/bus.h"
#include "../includes/cartridge.h"
#include "../includes/ppu.h"
#include "../includes/cpu.h"

#define CPU_RAM_SIZE 1024 * 2
#define CPU_RAM_END  0x1FFF     // ending address space of cpu ram

static uint8_t cpu_ram[CPU_RAM_SIZE];

// read single byte from bus
uint8_t cpu_bus_read(uint16_t position)
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
      return ppu_cpu_read( 0x2000 | (position & 0x7) );
   }

   return 0xFF;
}

// write single byte to bus
void cpu_bus_write(uint16_t position, uint8_t data)
{
   // accessing 2 kb cpu ram address space
   if ( position <= CPU_RAM_END )
   {
      cpu_ram[position & 0x7FF] = data;
   }
   // accessing ppu registers
   else if ( position >= CPU_PPU_REG_START && position <= CPU_PPU_REG_END )
   {
      ppu_cpu_write( 0x2000 | (position & 0x7), data );
   }
}

// read 2 bytes from bus, unpack from little endian
uint16_t cpu_bus_read_u16(uint16_t position)
{
   uint8_t lo = cpu_bus_read(position);
   uint16_t hi = cpu_bus_read(position + 1) << 8;

   return hi | lo;
}

// write 2 bytes to bus, packed in little endian
void cpu_bus_write_u16(uint16_t position, uint16_t data)
{
   uint8_t lo = data & 0x00FF;
   cpu_bus_write(position, lo);
   
   uint8_t hi = ( data & 0xFF00 ) >> 8;
   cpu_bus_write(position + 1, hi);
}