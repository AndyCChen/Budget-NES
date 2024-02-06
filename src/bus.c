#include <stdint.h>
#include <stdio.h>

#include "../includes/bus.h"
#include "../includes/cpu_ram.h"
#include "../includes/cartridge.h"

// read single byte from bus
uint8_t bus_read(uint16_t position)
{
   // accessing 2 kb cpu ram address space
   if ( position <= 0x1FFF )
   {
      return cpu_ram_read(position);
   }
   // accessing program rom address space
   else if ( position >= 0x8000 )
   {
      return cartridge_read(position);
   }

   return 0xFF;
}

// write single byte to bus
void bus_write(uint16_t position, uint8_t data)
{
   // accessing 2 kb cpu ram address space
   if ( position <= 0x1FFF )
   {
      cpu_ram_write(position, data);
   }
}

// read 2 bytes from bus, unpack from little endian
uint16_t bus_read_u16(uint16_t position)
{
   uint8_t lo = bus_read(position);
   uint16_t hi = bus_read(position + 1) << 8;

   return hi | lo;
}

// write 2 bytes to bus, packed in little endian
void bus_write_u16(uint16_t position, uint16_t data)
{
   uint8_t lo = data & 0x00FF;
   bus_write(position, lo);
   
   uint8_t hi = ( data & 0xFF00 ) >> 8;
   bus_write(position + 1, hi);
}