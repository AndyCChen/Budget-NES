#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../includes/ppu.h"
#include "../includes/cartridge.h"
#include "../includes/cpu.h"
#include "../includes/bus.h"

// cpu mapped addresses of PPU ports at 0x2000 - 0x2007 and 0x4041

#define PPUCTRL   0x2000 // write
#define PPUMASK   0x2001 // write
#define PPUSTATUS 0x2002 // read
#define OAMADDR   0x2003 // write
#define OAMDATA   0x2004 // read/write
#define PPUSCROLL 0x2005 // write x2
#define PPUADDR   0x2006 // write x2
#define PPUDATA   0x2007 // read/write
#define OAMDMA    0x4041 // write

#define PALETTE_START 0x3F00

// internal registers

static uint8_t ppu_control = 0;
static uint8_t ppu_mask = 0;
static uint8_t ppu_status = 0xA0;
static uint8_t oam_address = 0;
static uint8_t oam_data = 0;

static bool write_toggle = false; // false: first write, true: second write for PPUADDR and PPUSCROLL port, set to false when ppu_status is read
static uint8_t x_register = 0;    // fine x scroll (3 bits)
static uint16_t t_register = 0;   // 15-bit temporary vram address
static uint16_t v_register = 0;   // current vram address that when used through port $2007 to access ppu memory only 14 bits are used, but it is otherwise a 15 bit register

static uint8_t  nametable_byte;
static uint8_t  pattern_tile_lo_bits;
static uint8_t  pattern_tile_hi_bits;
static uint8_t  attribute_byte;
static uint16_t u16_shift_register1;
static uint16_t u16_shift_register2;
static uint8_t  u8_shift_register1;
static uint8_t  u8_shift_register2;

static bool odd_even_flag = false; // false: on a odd frame, true: on a even frame

// bus

static uint8_t vram_buffer = 0;   // internal buffer that holds contents being read from vram
static uint8_t open_bus = 0;

// memory

static uint8_t palette_ram[32];
static uint8_t oam_ram[256];
static uint8_t secondary_oam_ram[32];

// track current scanline and cycles

static uint16_t scanline = 261;
static uint16_t cycle = 0;

// render pipeline events

static void rest_cycle(void);
static void fetch_nametable(void);
static void fetch_attribute(void);
static void fetch_pattern_table(void);
static void increment_v_horizonal(void);
static void increment_v_vertical(void);
static void secondary_oam_clear(void);
static void evaluate_sprite(void);

void ppu_cycle(void)
{
   // scanline 0-239 (i.e 240 scanlines) are the visible scanlines to the display
   if (scanline >= 0 && scanline <= 239)
   {
      

   }
   else if (scanline >= 241 && scanline <= 260) // vertical blank scanlines
   {

   }
   else if (scanline == 261) // pre-render scanline
   {

   }
   else // scanline 240, idle scanline
   {

   }
}

void ppu_port_write(uint16_t position, uint8_t data)
{
   switch(position)
   {
      case PPUCTRL:
         ppu_control = data;
         
         // transfer bits 0-1 of ppu_control to bits 10-11 of t_register
         uint16_t NN = (ppu_control & 0x3) << 10;
         t_register = t_register & ~(0x0C00); // clear bits 10-11 of t_register before transfering bits 0-1
         t_register = t_register | NN;
         break;
      case PPUMASK:
         ppu_mask = data;
         break;
      case OAMADDR:
         oam_address = data;
         break;
      case PPUSCROLL:
         if (!write_toggle) // first write
         {
            x_register = data & 0x7; // bits 0-2 stored into x_register

            t_register = t_register & ~(0x001F); // clear bits 0-4 before transfer
            t_register = t_register | ( (data & 0xF8) >> 3 ); // bits 3-7 stored into bits 0-4 of t_register
         }
         else // second write
         {
            t_register = t_register & ~(0x73E0); // clear bits 5-9 and bits 12-14 before transfer
            t_register = t_register | ( (data & 0x7) << 12 ); // bits 0-2 stored into bits 12-14 of t_register
            t_register = t_register | ( (data & 0xF8) << 2 ); // bits 3-7 stored into bits 5-9 of t_register
         }

         write_toggle = !write_toggle;
         break;
      case PPUADDR:
         if (!write_toggle)
         {
            // writing high byte (first write)
            t_register = t_register & ~(0x7F00);
            t_register = t_register | (data & 0x3F) << 8; 
         }
         else
         {
            // writing low byte (second write)
            t_register = t_register & ~(0x00FF);
            t_register = t_register | data;
            v_register = t_register;
         }

         write_toggle = !write_toggle;
         break;
      case OAMDMA: // todo: handle clock cycles
      {
         uint16_t read_address = data << 8;
         for (size_t i = 0; i < 256; ++i)
         {
            oam_data = cpu_bus_read(read_address + i);
            oam_ram[oam_address] = oam_data;
            oam_address += 1;
         }
         break;
      }
      case OAMDATA:
         oam_data = data;
         oam_ram[oam_address] = oam_data;
         oam_address += 1;
         break;
      case PPUDATA:
         if ( (v_register & 0x3FFF) >= PALETTE_START )
         {
            // writing to palette ram
            palette_ram[v_register & 0x1F] = data;
         }
         else
         {
            cartridge_ppu_write(v_register & 0x3FFF, data);
         }
         
         v_register = ( v_register + ((ppu_control & 4) ? 32 : 1) ) & 0x7FFF; // increment vram address by 1 if bit 2 of control register is 0, else increment by 32
         break;
   }

   open_bus = data; // writes to any ppu ports loads a value into the I/O bus
}

uint8_t ppu_port_read(uint16_t position)
{
   switch(position)
   {
      case OAMDATA: // read/write
         oam_data = oam_ram[oam_address];
         open_bus = oam_data;
         break;
      case PPUDATA:
         open_bus = vram_buffer;
         vram_buffer = cartridge_ppu_read(v_register & 0x3FFF);
         
         // when reading palette, data is returned directly from palette ram rather than the internal read buffer
         if ( (v_register & 0x3FFF) >= PALETTE_START )
         {
            return palette_ram[v_register & 0x1F];
         }

         v_register += (ppu_control & 4) ? 32 : 1; // increment vram address by 1 if bit 2 of control register is clear, else increment by 32
         break;
      case PPUSTATUS: // read only
         open_bus = (ppu_status & 0xE0) | open_bus;
         write_toggle = false;
         ppu_status = ppu_status & ~(0x80); // clear vertical blank flag after read
         break;
   }

   return open_bus;
}

static void rest_cycle(void){} // function that does nothing


static void fetch_nametable(void)
{
   return cartridge_ppu_read( 0x2000 | (v_register & 0x0FFF) );
}

/*
 NN 1111 YYY XXX
 || |||| ||| +++-- high 3 bits of coarse X (x/4)
 || |||| +++------ high 3 bits of coarse Y (y/4)
 || ++++---------- attribute offset (960 bytes)
 ++--------------- nametable select
*/
static void fetch_attribute()
{
   //                                     nametable select         hi 3 bit of coarse y           hi 3 bit of coarse x
   uint16_t attribute_address = 0x23C0 | (v_register & 0xC00) | ( (v_register >> 4) & 0x38 ) | ( (v_register >> 2) & 0x7 );
   //                           0x23C0 means select from address space 0x2000 and up with a 960 byte offset. Attribute table is the last 64 bytes of our 1024 byte nametable

   attribute_byte = cartridge_ppu_read(attribute_address);
}

/* 
0HNNNN NNNNPyyy
|||||| |||||+++- T: Fine Y offset, the row number within a tile
|||||| ||||+---- P: Bit plane (0: less significant bit; 1: more significant bit)
||++++-++++----- N: Tile number from name table
|+-------------- H: Half of pattern table (0: "left"; 1: "right")
+--------------- 0: Pattern table is at $0000-$1FFF 
*/
static void fetch_pattern_table()
{
   static uint8_t bit_plane_toggle = 0; // 0: fetching less significant bit, 1: fetching most significant bit

   uint16_t pattern_tile_address =  ( (ppu_control & 0x10) << 8 )  | (nametable_byte << 4) | (bit_plane_toggle << 3) | ( (v_register >> 12) & 0x7 );
   bit_plane_toggle = ~bit_plane_toggle;

   if (bit_plane_toggle == 0) pattern_tile_lo_bits = cartridge_ppu_read(pattern_tile_address);
   else                       pattern_tile_hi_bits = cartridge_ppu_read(pattern_tile_address);
}

/**
 * Increments the coarse X bits of v_register with overflow toggling bit 10 which
 * selects the horizonal nametable
*/
static void increment_v_horizonal(void)
{
   if ( (v_register & 0x1F) == 31 ) // coarse X are bits 0-4 which can represent values 0-31, so overflow will happen if coarse X == 31 when we increment
   {
      v_register = v_register & (~0x1F); // wrap back down to zero on overflow
      v_register = v_register ^ 0x400; // toggle bit 10 on overflow
   }
   else
   {
      v_register += 1;
   }

   /* uint8_t coarse_x = (v_register & 0x1F) + 1;
   uint8_t overflow_bit = v_register & 0x20;
   (v_register ^ 0x400) & (overflow_bit << 5); */
}

/**
 * Increments the fine Y bits (3 bits) of v_register with overflow toggling bit 11 which
 * selects the vertical nametable.
*/
static void increment_v_vertical(void)
{
   if ( (v_register & 0x7000) == 0x7000 ) // fine y bits == 7 which means overflow will happen
   {
      v_register &= ~0x7000; // fine y bits wrap down to zero

      uint8_t coarse_y = (v_register >> 5) & 0x1F;

      // when fine y wraps down to zero, we increment coarse_y bits

      if (coarse_y == 29) // toggle bit 11 on overflow
      {
         coarse_y = 0;
         v_register ^= 0x800;
      }
      else if (coarse_y == 31) // bit 11 does not get toggled when set out of bounds, nametable rows are only index 0-29 (30 rows)
      {
         coarse_y = 0;
      }
      else
      {
         coarse_y += 1;
      }

      v_register &= ~0x3E0;
      v_register |= coarse_y << 5;
   }
   else
   {
      v_register += 0x1000;
   }
}