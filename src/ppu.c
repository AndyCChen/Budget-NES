#include <stdint.h>
#include <stdbool.h>

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

static uint8_t ppu_control = 0;
static uint8_t ppu_mask = 0;
static uint8_t ppu_status = 0xA0;
static uint8_t oam_address = 0;
static uint8_t oam_data = 0;

static bool write_toggle = false; // false: first write, true: second write for PPUADDR and PPUSCROLL port, set to false when ppu_status is read
static uint8_t x_register = 0; // fine x scroll (3 bits)
static uint16_t t_register = 0; // 15-bit temporary vram address
static uint16_t v_register = 0; // current vram address that when used through port $2007 to access ppu memory only 14 bits are used, but it is otherwise a 15 bit register

static uint8_t vram_hi_address = 0; // lo and hi addresses used to form the full effective vram address
static uint8_t vram_lo_address = 0;
static uint8_t vram_buffer = 0; // internal buffer that holds contents being read from vram

static uint8_t open_bus = 0;

// memory

#define PPU_RAM_SIZE 1024 * 2
#define PALETTE_SIZE 32
#define OAM_SIZE 256

static uint8_t vram[PPU_RAM_SIZE];
static uint8_t palette_ram[PALETTE_SIZE];
static uint8_t oam_ram[OAM_SIZE];

static uint16_t scanline = 261;
static uint16_t cycle = 0;

void ppu_cycle(void)
{
   // scanline 0-239 (i.e 240 scanlines) are the visible scanlines to the display
   if (scanline >= 0 && scanline <= 239)
   {
      

   }
   else if (scanline == 261) // pre-render scanline
   {

   }
}

void ppu_cpu_write(uint16_t position, uint8_t data)
{
   switch(position)
   {
      case PPUCTRL:
         ppu_control = data;
         
         // transfer bits 0-1 of ppu_control to bits 10-11 of t_register
         uint16_t NN = (ppu_control & 0x3) << 10;
         t_register = t_register & 0xF3FF; // clear bits 10-11 of t_register before transfering bits 0-1
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

            t_register = t_register & 0x7FE0; // clear bits 0-4 before transfer
            t_register = t_register | ( (data & 0xF8) >> 3 ); // bits 3-7 stored into bits 0-4 of t_register
         }
         else // second write
         {
            t_register = t_register & 0x0C1F; // clear bits 5-9 and bits 12-14 before transfer
            t_register = t_register | ( (data & 0x7) << 12 ); // bits 0-2 stored into bits 12-14 of t_register
            t_register = t_register | ( (data & 0xF8) << 2 ); // bits 3-7 stored into bits 5-9 of t_register
         }

         write_toggle = !write_toggle;
         break;
      case PPUADDR:
         if (!write_toggle)
         {
            // writing high byte (first write)
            vram_hi_address = data;
         }
         else
         {
            // writing low byte (second write)
            vram_lo_address = data;
            v_register = ( (vram_hi_address << 8) | vram_lo_address ) & 0x3FFF;
         }

         write_toggle = !write_toggle;
         break;
      case OAMDMA: // todo: handle clock cycles
         uint16_t read_address = data << 8;
         for (size_t i = 0; i < 256; ++i)
         {
            oam_data = cpu_bus_read(read_address | i);
            oam_ram[oam_address] = oam_data;
            oam_address += 1;
         }
         break;
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
            cartridge_ppu_write(v_register & 0x3FFF, data, vram);
         }
         
         v_register = ( v_register + ((ppu_control & 4) ? 32 : 1) ) & 0x7FFF; // increment vram address by 1 if bit 2 of control register is 0, else increment by 32
         break;
   }

   open_bus = data; // writes to any ppu ports loads a value into the I/O bus
}

uint8_t ppu_cpu_read(uint16_t position)
{
   switch(position)
   {
      case OAMDATA: // read/write
         oam_data = oam_ram[oam_address];
         open_bus = oam_data;
         break;
      case PPUDATA:
         open_bus = vram_buffer;
         vram_buffer = cartridge_ppu_read(v_register & 0x3FFF, vram);
         
         // when reading palette, data is returned directly from palette ram rather than the internal read buffer
         if ( v_register & 0x3FFF >= PALETTE_START )
         {
            return palette_ram[v_register & 0x1F];
         }

         v_register += (ppu_control & 4) ? 32 : 1; // increment vram address by 1 if bit 2 of control register is clear, else increment by 32
         break;
      case PPUSTATUS: // read only
         open_bus = (ppu_status & 0xE0) | open_bus;
         write_toggle = false;
         break;
   }

   return open_bus;
}