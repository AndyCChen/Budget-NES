#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vec3.h"
#include "vec4.h"

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
#define OAMDMA    0x4014 // write

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

static bool odd_even_flag = true; // false: on a odd frame, true: on a even frame

// bus

static uint8_t read_buffer = 0;   // internal buffer that holds contents being read from port 2007 that are non-pallete addresses
static uint8_t open_bus = 0;

// memory

static uint8_t palette_ram[32];
static uint8_t oam_ram[256];
static input_sprite_t secondary_oam_ram[8];
static output_sprite_t output_sprites[8]; // array of fetched sprites that will be rendered on the next scanline
static uint8_t number_of_sprites = 0;     // number of sprites to draw on the next scanline

// track current scanline and cycles

static uint16_t scanline = 261;
static uint16_t cycle = 0;

// 64 rgb colors for system_palette
static vec3 system_palette[64];

static bool oam_dma_scheduled = false;
static uint16_t oam_dma_address;

// retrieves palette index that is mirrored if necessary
static uint8_t get_palette_index(uint8_t index);
static uint8_t flip_bits_horizontally(uint8_t in);
static void sprite_evaluation(void);

void ppu_cycle(bool* nmi_flip_flop)
{
   uint8_t background_pixel = 0;
   uint8_t sprite_pixel = 0;

   // background tile rendering
   if ( (cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336) )
   {
      /**
       * Output pixel bit pattern
       * 43210
         |||||
         |||++- Pixel value from tile data, this is the index within a pallete.
         |++--- Palette number from attribute table or OAM, this is what pallete to use.
         +----- Background/Sprite select.
      */

      // PPU selects bits to output a final color index for a pixel, after which shift registers are shifted 1 bit 
      // and every 8 cycles the shift registers are reloaded with newly fetched data.

      uint8_t position = 15 - x_register; // position of the bit to select

      background_pixel = (tile_shift_register_lo >> position) & 0x1;              // set bit 0
      background_pixel |= ( (tile_shift_register_hi >> position) & 0x1 ) << 1;    // set bit 1

      position = 7 - x_register;

      background_pixel |= ( (attribute_shift_register_lo >> position) & 0x1 ) << 2; // set bit 2
      background_pixel |= ( (attribute_shift_register_hi >> position) & 0x1 ) << 3; // set bit 3

      // shift registers left by 1

      tile_shift_register_lo = tile_shift_register_lo << 1;
      tile_shift_register_hi = tile_shift_register_hi << 1;

      // the bit in the 1 bit latches are shifted into the attribute shift registers

      attribute_shift_register_lo = attribute_shift_register_lo << 1;
      attribute_shift_register_lo |= attribute_1_bit_latch_x;          
      attribute_shift_register_hi = attribute_shift_register_hi << 1;
      attribute_shift_register_hi |= attribute_1_bit_latch_y;
   }

   int active_sprite = -1;
   //if (scanline == 30) ppu_status |= 0x40;
   // sprite rendering
   if (cycle >= 1 && cycle <= 256 && scanline <= 239) 
   {
      bool sprite_found = false;

      // search for the first in range opaque sprite pixel on the horizontal axis
      for (int i = 0; i < 8; ++i)
      {
         if ( cycle >= output_sprites[i].x_position + 1 && cycle - (output_sprites[i].x_position + 1) <= 8 /*&& scanline != 0*/ )
         {
            if (!sprite_found)
            {
               // contruct 4 bit pallete index with pattern table bitplanes and attribute bytes of the sprite
               sprite_pixel =  (output_sprites[i].lo_bitplane >> 7) & 0x1;
               sprite_pixel |= ( (output_sprites[i].hi_bitplane >> 7) & 0x1 ) << 1;
               sprite_pixel |= (output_sprites[i].attribute & 0x3) << 2;

               if ( (sprite_pixel & 0x3) != 0 ) // only set sprite found to true if the sprite pixel found is not transparent
               {
                  sprite_found = true;
                  active_sprite = i;
               }
            }

            // shift bitplanes once they have been used to render a pixel
            output_sprites[i].lo_bitplane = output_sprites[i].lo_bitplane << 1;
            output_sprites[i].hi_bitplane = output_sprites[i].hi_bitplane << 1;
         }
      }
   }

   // scanline 0-239 (i.e 240 scanlines) are the visible scanlines to the display
   if (scanline <= 239)
   {
      uint8_t output_pixel = 0;

      if (ppu_mask & 0x18) // check if rendering is enabled
      {
         scanline_lookup[cycle]();

         if (cycle == 1)
         {
            sprite_clear_secondary_oam(); // for simplicity, initialize secondary oam all cycle 1 of ppu visible scanlines
         }

         if (cycle == 65)
         {
            sprite_evaluation();         // for simplicity, do sprite evaluation all in 1 ppu cycle during cycle 65 of a visible scanline
         }

         // check if rendering of background or sprite pixels are disabled, if disabled just set color to transparent background color
         if ( (ppu_mask & 0x08) == 0 )
         {
            background_pixel = 0;
         }
         else if ( (ppu_mask & 0x10) == 0 )
         {
            sprite_pixel = 0;
         }
      }

      if (cycle >= 1 && cycle <= 256)
      {
         if (cycle <= 8)
         {
            // hide background pixels on leftmost 8 pixels of screen
            if ((ppu_mask & 0x2) == 0)
            {
               background_pixel &= 0xC;
            }
         }

         // no active sprite for this pixel so we just choose from the background
         if (active_sprite == -1)
         {
            if ( (background_pixel & 0x3) == 0 ) // transparent pixels will display colors at 0x3F00
            {
               output_pixel = 0;
            }
            else
            {
               output_pixel = background_pixel;
            }
         }
         // active sprite present so we must determine whether to render the background or sprite
         else
         {
            if (cycle <= 8)
            {
               // hide sprite pixels on leftmost 8 pixels of screen
               if ((ppu_mask & 0x4) == 0)
               {
                  sprite_pixel &= 0xC;
               }
            }

            // bg and sp are background and sprite color indices within a palette, 0 means that color is the transparent background color
            uint8_t bg = background_pixel & 0x3;
            uint8_t sp = sprite_pixel & 0x3;

            // 0: sprite is in front of background, 1: sprite is behind background
            uint8_t sp_priority = (output_sprites[active_sprite].attribute & 0x20) >> 5;

            if (bg == 0 && sp == 0)      output_pixel = 0;
            else if (bg == 0 && sp != 0) output_pixel = 0x10 | sprite_pixel;
            else if (bg != 0 && sp == 0) output_pixel = background_pixel;
            else                         output_pixel = (sp_priority) ? background_pixel : (0x10 | sprite_pixel);

            // check for sprite 0 hit
            if ( output_sprites[active_sprite].sprite_id == 0 )
            {
               if ( sprite_pixel != 0 &&  background_pixel != 0 )
               {
                  ppu_status |= 0x40;
               }
            }
         }
         //output_pixel = 0x0 | (output_pixel & 0x3);

         uint8_t palette_index = palette_ram[ output_pixel & 0x1F ] ;
         set_viewport_pixel_color( scanline, cycle - 1, system_palette[palette_index & 0x3F] );
      }
   }
   else if (scanline >= 240 && scanline <= 260) // vertical blank scanlines
   {
      if (scanline == 241 && cycle == 1)
		{
         display_update_color_buffer(); // update color buffer after visible scanlines are finished rendering

         if (ppu_control & 0x80)
         {
            *nmi_flip_flop = true;
         }

         ppu_status |= 0x80; // set VBlank flag on cycle 1 of scanline 241  
      }

      // ppu performs no memory accesses during vertical blank scanlines so we just do nothing here
   }
   else // pre-render scanline 261
   {
      if (cycle == 1)
      {
         odd_even_flag = !odd_even_flag;
         ppu_status &= ~0xE0; // clear VBlank and sprite 0 flag on cycle 1 of scanline 261
      }

      if (cycle >= 280 && cycle <= 304)
      {
         if (ppu_mask & 0x18) 
				transfer_t_vertical(); // reload vertical scroll bits if rendering enabled
      }
      else if (cycle == 339)
      {
         if (ppu_mask & 0x18 && odd_even_flag == false) 
				cycle = 340; // frames are 1 cycle shorted every odd frame 
      }

      if (ppu_mask & 0x18) 
			scanline_lookup[cycle](); // execute function from lookup table if rendering enabled
   }

   cycle++;
   if (cycle == 341) // finish processing 341 cycles of 1 scanline, move onto the next scanline
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

            write_toggle = true;
         }
         else // second write
         {
            t_register = t_register & ~(0x73E0); // clear bits 5-9 and bits 12-14 before transfer
            t_register = t_register | ( (data & 0x7) << 12 ); // bits 0-2 stored into bits 12-14 of t_register
            t_register = t_register | ( (data & 0xF8) << 2 ); // bits 3-7 stored into bits 5-9 of t_register

            write_toggle = false;
         }

         break;
      case PPUADDR:
         if (!write_toggle)
         {
            // writing high byte (first write)
            t_register = t_register & ~(0x7F00);
            t_register = t_register | (data & 0x3F) << 8; 
            
            write_toggle = true;
         }
         else
         {
            // writing low byte (second write)
            t_register = t_register & ~(0x00FF);
            t_register = t_register | data;
            v_register = t_register;

            write_toggle = false;
         }

         break;
      case OAMDMA:
      {
			oam_dma_scheduled = true;
			oam_dma_address = data << 8;

         //uint16_t read_address = data << 8;
         //cpu_tick();
         //for (size_t i = 0; i < 256; ++i)
         //{
         //   cpu_tick();
         //   oam_data = cpu_bus_read(read_address + i);
         //   oam_ram[oam_address] = oam_data;
         //   oam_address += 1;
         //}
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
            palette_ram[get_palette_index( v_register & 0x1F )] = data;
         }
         else
         {
            cartridge_ppu_write(v_register & 0x3FFF, data);
         }

         if (ppu_control & 0x4)
         {
            v_register += 32;
         }
         else
         {
            v_register += 1;
         }
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
         open_bus = read_buffer;
         read_buffer = cartridge_ppu_read(v_register);
         
         // when reading palette, data is returned directly from palette ram rather than the internal read buffer
         if ( (v_register & 0x3FFF) >= PALETTE_START )
         {
            open_bus = palette_ram[ get_palette_index(v_register & 0x1F) ];
         }

         if (ppu_control & 0x4)
         {
            v_register += 32;
         }
         else
         {
            v_register += 1;
         }

         break;
      case PPUSTATUS: // read only
         open_bus = (ppu_status & 0xE0) | (open_bus & 0x1F); // load ppu status onto bits 7-5 of the open bus
         write_toggle = false;
         ppu_status &= ~0x80; // clear vertical blank flag after read
         break;
   }

   return open_bus;
}

void rest_cycle(void){return;} // function that does nothing to fill gaps inside the render event lookup table


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
   uint16_t attribute_address = 0x23C0 | (v_register & 0x0C00) | ( (v_register >> 4) & 0x38 ) | ( (v_register >> 2) & 0x07 );
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
   uint16_t pattern_tile_address =  ( (ppu_control & 0x10) << 8 )  | (nametable_byte << 4) | ( (v_register >> 12) & 0x7 );
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

   // load 1 bit latches with a bit selected by bit 1 of coarse x and y
   uint8_t x_bit = (v_register >> 1) & 0x1;
   uint8_t y_bit = (v_register >> 6) & 0x1;
   

   uint8_t position = x_bit * 2 + y_bit * 4;
   attribute_1_bit_latch_x = ( attribute_byte & (1 << position) ) >> position;
   attribute_1_bit_latch_y = ( attribute_byte & ( 1 << (position + 1) ) ) >> (position + 1);

   if ( (v_register & 0x1F) == 31 ) // coarse X are bits 0-4 which can represent values 0-31, so overflow will happen if coarse X == 31 when we increment
   {
      v_register = v_register & (~0x1F); // wrap back down to zero on overflow
      v_register = v_register ^ 0x400;   // toggle bit 10 on overflow to switch nametables
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
         v_register ^= 0x800; // switch vertical nametables
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
   v_register = (v_register & ~0x41F) | (t_register & 0x41F);
}

/**
 * copy vertical related bits in t_register into v_register
*/
void transfer_t_vertical(void)
{
   v_register = (v_register & ~0x7BE0) | (t_register & 0x7BE0);
}

void sprite_clear_secondary_oam(void)
{
   for (size_t i = 0; i < 8; ++i)
   {
      secondary_oam_ram[i].sprite_id  = 0xFF;
      secondary_oam_ram[i].tile_id    = 0xFF;
      secondary_oam_ram[i].y_coord    = 0xFF;
      secondary_oam_ram[i].attribute  = 0xFF;
      secondary_oam_ram[i].x_position = 0xFF;
   }
}

bool ppu_scheduled_oam_dma(void)
{
	bool temp = oam_dma_scheduled;
	oam_dma_scheduled = false;
	return temp;
}

void ppu_handle_oam_dma(void)
{
	for (uint16_t i = 0; i < 256; ++i)
	{
		cpu_tick();
		oam_data = cpu_bus_read(oam_dma_address + i);
		oam_ram[oam_address] = oam_data;
		oam_address += 1;
	}
}

static void sprite_evaluation(void)
{
   uint8_t secondary_oam_index = 0;
   uint8_t sprite_id = 0;
   number_of_sprites = 0;
   while ( true )
   {
      uint8_t y_coord = oam_ram[oam_address];
      if ( secondary_oam_index < 8 )
      {
         secondary_oam_ram[secondary_oam_index].sprite_id = sprite_id;
         secondary_oam_ram[secondary_oam_index].y_coord = y_coord;

         if ( (scanline - y_coord) >= 0 && (scanline - y_coord) < ((ppu_control & 0x20) ? 16 : 8) ) // if sprite is in y range, copy rest of sprite data into secondary oam
         {
            secondary_oam_ram[secondary_oam_index].tile_id    = oam_ram[oam_address + 1]; // tile index
            secondary_oam_ram[secondary_oam_index].attribute  = oam_ram[oam_address + 2]; // attributes
            secondary_oam_ram[secondary_oam_index].x_position = oam_ram[oam_address + 3]; // x position
            secondary_oam_index += 1; // increment to next free location in secondary oam
            number_of_sprites += 1;
         }
      }

      if ( (oam_address + 4) > 255 ) // finish sprite evaluation when all sprites in oam_ram has been scanned
      {
         break;
      }
      else
      {
         oam_address += 4;          // else increment oam_address by 4 bytes and continue evaluation
         sprite_id += 1;
      }
   }
   // todo: sprite overflow detection
}

/* 
0HNNNN NNNNPyyy
|||||| |||||+++- T: Fine Y offset, the row number within a tile
|||||| ||||+---- P: Bit plane (0: less significant bit; 1: more significant bit)
||++++-++++----- N: Tile number from name table
|+-------------- H: Half of pattern table (0: "left"; 1: "right")
+--------------- 0: Pattern table is at $0000-$1FFF 
*/

// we cheat a little here and fetch sprites all on a single ppu cycle for simplicity
void fetch_sprites(void)
{
	static uint8_t i = 0;
   oam_address = 0;
   
	uint8_t sprite_fine_y        = (uint8_t) (scanline - secondary_oam_ram[i].y_coord); // row within a sprite
   uint8_t tile_number          = secondary_oam_ram[i].tile_id;
   output_sprites[i].sprite_id  = secondary_oam_ram[i].sprite_id;
   output_sprites[i].attribute  = secondary_oam_ram[i].attribute;
   output_sprites[i].x_position = secondary_oam_ram[i].x_position;

	uint16_t pattern_tile_address_lo = 0;

   // using 8 by 8 sprites
   if ((ppu_control & 0x20) == 0)
   {
      // sprite flipped vertically
      if (output_sprites[i].attribute & 0x80)
      {
         sprite_fine_y = 7 - sprite_fine_y;
      }

      // fetching lo bitplane
      pattern_tile_address_lo = ( (ppu_control & 0x8) << 9 )  | (tile_number << 4) | (sprite_fine_y & 0x7);
   }
   // using 8 by 16 sprites
   else                           
   {
      // sprite flipped vertically
      if (output_sprites[i].attribute & 0x80)
      {
         // fetching bottom tile
         if (sprite_fine_y < 8)
         {
            sprite_fine_y = 7 - sprite_fine_y;
            pattern_tile_address_lo = ( (tile_number & 0x1) << 12 ) | (( (tile_number & 0xFE) + 1 ) << 4) | (sprite_fine_y & 0x7);
         }
         // fetching upper tile
         else
         {
            sprite_fine_y = 7 - sprite_fine_y;
            pattern_tile_address_lo = ( (tile_number & 0x1) << 12 ) | (( (tile_number & 0xFE) ) << 4) | (sprite_fine_y & 0x7);
         }
      }
      // sprite is NOT flipped vertically
      else
      {
         // fetching upper tile
         if (sprite_fine_y < 8)
         {
            pattern_tile_address_lo = ( (tile_number & 0x1) << 12 ) | (( tile_number & 0xFE ) << 4) | (sprite_fine_y & 0x7);
         }
         // fetching bottom tile
         else
         {
            pattern_tile_address_lo = ( (tile_number & 0x1) << 12 ) | (( (tile_number & 0xFE) + 1 ) << 4) | (sprite_fine_y & 0x7);
         }
      }	
   }

	output_sprites[i].lo_bitplane = cartridge_ppu_read(pattern_tile_address_lo);
	output_sprites[i].hi_bitplane = cartridge_ppu_read(pattern_tile_address_lo + 8);

   // sprite flipped horizontally
   if (output_sprites[i].attribute & 0x40)
   {
      output_sprites[i].lo_bitplane = flip_bits_horizontally( output_sprites[i].lo_bitplane );
      output_sprites[i].hi_bitplane = flip_bits_horizontally( output_sprites[i].hi_bitplane );
   }

   // when there are less than 8 sprites on the next scanline, the remaining fetches have their color index replaced with the transparent background color
   if (i >= number_of_sprites)
   {
      output_sprites[i].lo_bitplane = 0;
      output_sprites[i].hi_bitplane = 0;
   }
      
	i = (i + 1) & 0x7;
}

#define PALETTE_SIZE 192

bool ppu_load_palettes_from_file(const char* path)
{
   FILE *file = fopen(path, "rb");

   if (file == NULL)
   {
      printf("Cannot open .pal file!\n");
      return false;
   }

   uint8_t buffer[PALETTE_SIZE];
   size_t bytes_read = fread(buffer, sizeof(buffer[0]), PALETTE_SIZE, file);

   if (bytes_read != PALETTE_SIZE)
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

void ppu_load_default_palettes()
{
	const uint8_t colors[] =
	{
		 0x52, 0x52, 0x52, 0x01, 0x1A, 0x51, 0x0F, 0x0F, 0x65, 0x23, 0x06, 0x63, 0x36, 0x03, 0x4B, 0x40,
		 0x04, 0x26, 0x3F, 0x09, 0x04, 0x32, 0x13, 0x00, 0x1F, 0x20, 0x00, 0x0B, 0x2A, 0x00, 0x00, 0x2F,
		 0x00, 0x00, 0x2E, 0x0A, 0x00, 0x26, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0xA0, 0xA0, 0xA0, 0x1E, 0x4A, 0x9D, 0x38, 0x37, 0xBC, 0x58, 0x28, 0xB8, 0x75, 0x21, 0x94, 0x84,
		 0x23, 0x5C, 0x82, 0x2E, 0x24, 0x6F, 0x3F, 0x00, 0x51, 0x52, 0x00, 0x31, 0x63, 0x00, 0x1A, 0x6B,
		 0x05, 0x0E, 0x69, 0x2E, 0x10, 0x5C, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0xFE, 0xFF, 0xFF, 0x69, 0x9E, 0xFC, 0x89, 0x87, 0xFF, 0xAE, 0x76, 0xFF, 0xCE, 0x6D, 0xF1, 0xE0,
		 0x70, 0xB2, 0xDE, 0x7C, 0x70, 0xC8, 0x91, 0x3E, 0xA6, 0xA7, 0x25, 0x81, 0xBA, 0x28, 0x63, 0xC4,
		 0x46, 0x54, 0xC1, 0x7D, 0x56, 0xB3, 0xC0, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0xFE, 0xFF, 0xFF, 0xBE, 0xD6, 0xFD, 0xCC, 0xCC, 0xFF, 0xDD, 0xC4, 0xFF, 0xEA, 0xC0, 0xF9, 0xF2,
		 0xC1, 0xDF, 0xF1, 0xC7, 0xC2, 0xE8, 0xD0, 0xAA, 0xD9, 0xDA, 0x9D, 0xC9, 0xE2, 0x9E, 0xBC, 0xE6,
		 0xAE, 0xB4, 0xE5, 0xC7, 0xB5, 0xDF, 0xE4, 0xA9, 0xA9, 0xA9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	vec3 rgb_color;
	uint16_t position = 0;
	for (size_t i = 0; i < 64; ++i)
	{
		for (size_t j = 0; j < 3; ++j)
		{
			rgb_color[j] = colors[position++] / 255.0f;
		}

		glm_vec3_copy(rgb_color, system_palette[i]);
	}
}

static uint8_t get_palette_index(uint8_t index)
{
   switch (index)
   {
      case 0x10:
         return 0x00;
      case 0x14:
         return 0x04;
      case 0x18:
         return 0x08;
      case 0x1C:
         return 0x0C;
      default:
         return index;
   }
}

static uint8_t flip_bits_horizontally(uint8_t in)
{
   in = (in & 0xF0) >> 4 | (in & 0x0F) << 4;
   in = (in & 0xCC) >> 2 | (in & 0x33) << 2;
   in = (in & 0xAA) >> 1 | (in & 0x55) << 1;
   return in;
}

void DEBUG_ppu_update_pattern_tables(vec4* p0, vec4* p1)
{
   //const uint8_t debug_palette[4] = {0x3F, 0x00, 0x10, 0x20};

   for (uint8_t tile_row = 0; tile_row < 16; ++tile_row)
   {
      for (uint8_t tile_col = 0; tile_col < 16; ++tile_col)
      {
         uint8_t tile_number = (tile_row * 16) + tile_col;
         
         for (uint8_t fine_y = 0; fine_y < 8; ++fine_y)
         {
            uint8_t p0_lo = cartridge_ppu_read( (tile_number << 4) | fine_y );
            uint8_t p0_hi = cartridge_ppu_read( (tile_number << 4) | (1 << 3) | fine_y );

            uint8_t p1_lo = cartridge_ppu_read( (1 << 12) | (tile_number << 4) | fine_y );
            uint8_t p1_hi = cartridge_ppu_read( (1 << 12) | (tile_number << 4) | (1 << 3) | fine_y );

            for (int fine_x = 0; fine_x < 8; ++fine_x)
            {
               vec3* p0_color = &system_palette[ palette_ram[( (p0_hi & 0x80) >> 6 ) | ( (p0_lo & 0x80) >> 7 )] ];
               vec3* p1_color = &system_palette[ palette_ram[( (p1_hi & 0x80) >> 6 ) | ( (p1_lo & 0x80) >> 7 )] ];

               uint32_t index = (tile_row * 128 * 8) + (tile_col * 8) + (fine_y * 128) + fine_x;

               p0[index][0] = p0_color[0][0];
               p0[index][1] = p0_color[0][1];
               p0[index][2] = p0_color[0][2];
               p0[index][3] = 1.0f;

               p1[index][0] = p1_color[0][0];
               p1[index][1] = p1_color[0][1];
               p1[index][2] = p1_color[0][2];
               p1[index][3] = 1.0f;

               p0_lo = p0_lo << 1;
               p0_hi = p0_hi << 1;

               p1_lo = p1_lo << 1;
               p1_hi = p1_hi << 1;
            }
         }
      }
   }
}

void ppu_reset(void)
{
   ppu_control = 0;
   ppu_mask = 0;
   write_toggle = false;
   read_buffer = 0;
   odd_even_flag = true;
   x_register = 0;
   t_register = 0;
	oam_dma_scheduled = false;
}

void ppu_init(void)
{
   ppu_control = 0;
   ppu_mask = 0;
   ppu_status = 0;
   oam_address = 0;
   oam_data = 0;
   write_toggle = false;
   x_register = 0;
   t_register = 0;
   v_register = 0;
   nametable_byte = 0;
   pattern_tile_lo_bits = 0;
   pattern_tile_hi_bits = 0;
   attribute_byte = 0;
   tile_shift_register_hi = 0;
   tile_shift_register_lo = 0;
   attribute_shift_register_hi = 0;
   attribute_shift_register_lo = 0;
   attribute_1_bit_latch_x = 0;
   attribute_1_bit_latch_y = 0;
   odd_even_flag = true;
   read_buffer = 0;
   open_bus = 0;
   number_of_sprites = 0;
   scanline = 261;
   cycle = 0;
	oam_dma_scheduled = false;
}
