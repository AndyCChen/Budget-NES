#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "vec3.h"

#include "../includes/ppu.h"
#include "../includes/cartridge.h"
#include "../includes/cpu.h"
#include "../includes/bus.h"
#include "../includes/ppu_renderer_lookup.h"
#include "../includes/display.h"

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

static uint8_t  nametable_byte = 0;
static uint8_t  pattern_tile_lo_bits = 0;
static uint8_t  pattern_tile_hi_bits = 0;
static uint8_t  attribute_byte = 0;
static uint16_t tile_shift_register_lo = 0;
static uint16_t tile_shift_register_hi = 0;
static uint8_t  attribute_shift_register_lo = 0;
static uint8_t  attribute_shift_register_hi = 0;
static uint8_t  attribute_1_bit_latch_x = 0;     // 1 bit value selected by bit 1 of coarse_x
static uint8_t  attribute_1_bit_latch_y = 0;     // 1 bit value selected by bit 1 of coarse_y

static bool odd_even_flag = false; // false: on a odd frame, true: on a even frame
static bool nmi_has_occured = false;

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

// 64 rgb colors for system_palette
static vec3 system_palette[64];

// retrieves palette index that is mirrored if necessary
static uint8_t get_palette_index(uint8_t index);

void ppu_cycle(void)
{
   uint8_t output_pixel = 0; // final 5 bit output index into palette ram for a pixel color
   if ( (cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336) )
   {
      uint8_t position = 15 - x_register; // position of the bit to select

      /**
       * Output pixel bit pattern
       * 43210
         |||||
         |||++- Pixel value from tile data
         |++--- Palette number from attribute table or OAM
         +----- Background/Sprite select
      */

      output_pixel = (tile_shift_register_lo >> position) & 0x1;              // set bit 0
      output_pixel |= ( (tile_shift_register_hi >> position) & 0x1 ) << 1;    // set bit 1

      position = 7 - x_register;

      output_pixel |= ( (attribute_shift_register_lo >> position) & 0x1 ) << 2; // set bit 2
      output_pixel |= ( (attribute_shift_register_hi >> position) & 0x1 ) << 3; // set bit 3

      // shift registers left by 1

      tile_shift_register_lo = tile_shift_register_lo << 1;
      tile_shift_register_hi = tile_shift_register_hi << 1;

      // the bit in the 1 bit latches are shifted into the attribute shift registers

      attribute_shift_register_lo = attribute_shift_register_lo << 1;
      attribute_shift_register_lo |= attribute_1_bit_latch_x;          
      attribute_shift_register_hi = attribute_shift_register_hi << 1;
      attribute_shift_register_hi |= attribute_1_bit_latch_y;
   }

   // scanline 0-239 (i.e 240 scanlines) are the visible scanlines to the display
   if (scanline <= 239)
   {
      scanline_lookup[cycle](); // execute function from lookup table

      if (cycle >= 1 && cycle <= 256)
      {
         //set_pixel_color()
         uint8_t palette_index = palette_ram[ get_palette_index(output_pixel) ] & 0x3F;
         set_pixel_color( scanline, cycle - 1, system_palette[palette_index] );
      }
   }
   else if (scanline >= 240 && scanline <= 260) // vertical blank scanlines
   {
      if (scanline == 240 && cycle == 0)
      {
         if (ppu_control & 0x80)
         {
            nmi_has_occured = true;
         }
      }

      if (scanline == 260 && cycle == 340)
      {
         if (ppu_control & 0x80)
         {
            nmi_has_occured = false;
         }
      }

      if (scanline == 241 && cycle == 1)
      {  
         ppu_status |= 0x80; // set VBlank flag on cycle 1 of scanline 241  
      }

      // ppu performs no memory accesses during vertical blank scanlines so we just do nothing here
   }
   else // pre-render scanline 261
   {
      if (cycle == 1)
      {
         ppu_status &= ~0x80; // clear VBlank flag on cycle 1 of scanline 261
      }

      if (cycle >= 280 && cycle <= 304)
      {
         transfer_t_vertical();
      }

      scanline_lookup[cycle](); // execute function from lookup table
   }

   cycle++;
   if (cycle > 340) // finish processing 341 cycles of 1 scanline, move onto the next scanline
   {
      cycle = 0;
      scanline++;
      scanline = scanline % 262;
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
            open_bus = palette_ram[v_register & 0x1F];
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

void rest_cycle(void){} // function that does nothing to fill gaps inside the render event lookup table


void fetch_nametable(void)
{
   nametable_byte = cartridge_ppu_read( 0x2000 | (v_register & 0x0FFF) );
}

/*
 NN 1111 YYY XXX
 || |||| ||| +++-- high 3 bits of coarse X (x/4)
 || |||| +++------ high 3 bits of coarse Y (y/4)
 || ++++---------- attribute offset (960 bytes)
 ++--------------- nametable select
*/
void fetch_attribute()
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
void fetch_pattern_table_lo()
{
   uint16_t pattern_tile_address =  ( (ppu_control & 0x10) << 8 )  | (nametable_byte << 4) | (0 << 3) | ( (v_register >> 12) & 0x7 );
   pattern_tile_lo_bits = cartridge_ppu_read(pattern_tile_address);
}

void fetch_pattern_table_hi()
{
   uint16_t pattern_tile_address =  ( (ppu_control & 0x10) << 8 )  | (nametable_byte << 4) | (1 << 3) | ( (v_register >> 12) & 0x7 );
   pattern_tile_hi_bits = cartridge_ppu_read(pattern_tile_address);
}

/**
 * Increments the coarse X bits of v_register with overflow toggling bit 10 which
 * selects the horizonal nametable
*/
void increment_v_horizontal(void)
{
   // load shift registers with new data
   tile_shift_register_lo |= pattern_tile_lo_bits;
   tile_shift_register_hi |= pattern_tile_hi_bits;

   // load 1 bit latches with a bit selected by coarse x and y
   uint8_t x_bit = v_register & 0x1;
   uint8_t y_bit = (v_register >> 5) & 0x1;
   

   uint8_t position = x_bit * 2 + y_bit * 4;
   attribute_1_bit_latch_x = ( attribute_byte & (1 << position) ) >> position;
   attribute_1_bit_latch_y = ( attribute_byte & ( 1 << (position + 1) ) ) >> (position + 1);

   if ( (v_register & 0x1F) == 31 ) // coarse X are bits 0-4 which can represent values 0-31, so overflow will happen if coarse X == 31 when we increment
   {
      v_register = v_register & (~0x1F); // wrap back down to zero on overflow
      v_register = v_register ^ 0x400; // toggle bit 10 on overflow
   }
   else
   {
      v_register += 1;
   }
}

/**
 * Increments the fine Y bits (3 bits) of v_register with overflow toggling bit 11 which
 * selects the vertical nametable.
*/
void increment_v_vertical(void)
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

      v_register = (v_register & ~0x3E0) | (coarse_y << 5);
   }
   else
   {
      v_register += 0x1000;
   }
}

void increment_v_both(void)
{
   increment_v_horizontal();
   increment_v_vertical();
}

/**
 * copy horizonal related bits in t_register into v_register
*/
void transfer_t_horizontal(void)
{
   v_register = (v_register & ~0x41F) | (t_register & 0x41F) ;
}

/**
 * copy vertical related bits in t_register into v_register
*/
void transfer_t_vertical(void)
{
   v_register = (v_register & ~0x7BE0) | (t_register & 0x7BE0);
}

void fetch_sprite_lo(void)
{

}

void fetch_sprite_hi(void)
{

}

bool ppu_load_palettes(const char* path)
{
   size_t size = 192;

   FILE *file = fopen(path, "rb");

   if (file == NULL)
   {
      fclose(file);
      printf("Cannot open .pal file!\n");
      return false;
   }

   uint8_t buffer[size];
   size_t bytes_read = fread(buffer, 1, size, file);

   if (bytes_read != size)
   {
      fclose(file);
      printf("Invalid .pal file!\n");
      return false;
   }

   vec3 rgb_color;
   size_t position = 0;
   for (size_t i = 0; i < 64; ++i)
   {
      for (size_t j = 0; j < 3; ++j)
      {
         rgb_color[j] = buffer[position++] / 255.0f;
      }

      glm_vec3_copy(rgb_color, system_palette[i]);
   }

   fclose(file);
   return true;
}

static uint8_t get_palette_index(uint8_t index)
{
   switch (index)
   {
      case 0x04:
      case 0x08:
      case 0x0C:
         return index & 0x00;
      case 0x10:
         return index & 0x00;
      case 0x14:
         return index & 0x04;
      case 0x18:
         return index & 0x08;
      case 0x1C:
         return index & 0x0C;
      default:
         return index & 0x1F;
   }
}

bool get_nmi_status(void)
{
   return nmi_has_occured;
}