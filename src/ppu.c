#include <stdint.h>
#include <stdbool.h>

#include "../includes/ppu.h"
#include "../includes/cartridge.h"

#define PPU_RAM_SIZE 1024 * 2
#define PALETTE_SIZE 32

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

// ppu registers

static uint8_t ppu_control;
static uint8_t ppu_mask;
static uint8_t ppu_status;
static uint8_t oam_address;
static uint8_t oam_data;
static uint8_t ppu_scroll;
static uint8_t oam_dma;

static uint16_t vram_address; // effective 16-bit vram address
static uint8_t vram_hi_address; // lo and hi addresses used to form the full 16-bit effective vram address
static uint8_t vram_lo_address;
static uint8_t vram_buffer; // internal buffer that holds contents being read from vram


static bool write_toggle = false; // false: first write, true: second write for PPUADDR and PPUSCROLL port, set to false when ppu_status is read
static uint8_t open_bus;

// memory

static uint8_t vram[PPU_RAM_SIZE];
static uint8_t palette_ram[PALETTE_SIZE];

void ppu_cycle(void)
{
   
}

void ppu_cpu_write(uint16_t position, uint8_t data)
{
   switch(position)
   {
      case PPUCTRL: // write only
         ppu_control = data;
         break;
      case PPUMASK:
         ppu_mask = data;
         break;
      case OAMADDR:
         oam_address = data;
         break;
      case PPUSCROLL:
         ppu_scroll = data;
        // write_toggle = !write_toggle;
         break;
      case PPUADDR:
         if (!write_toggle)
         {
            // writing high byte (first write)
            vram_hi_address = data & 0x3F; // only get 6 high bits because ppu only addresses 0x0000 to 0x3FFF (6 hi bits + 8 low bits = 14 bits)
         }
         else
         {
            // writing low byte (second write)
            vram_lo_address = data;
            vram_address = (vram_hi_address << 8) | vram_lo_address;
         }

         write_toggle = !write_toggle;
         break;
      case OAMDMA:
         oam_dma = data;
         break;
      case OAMDATA: // read/write
         oam_data = data;
         break;
      case PPUDATA:
         if ( vram_address >= PALETTE_START )
         {
            // writing to palette ram
            palette_ram[vram_address] = data;
         }
         else
         {
            cartridge_ppu_write(vram_address, data);
         }
         
         vram_address += (ppu_control & 4) ? 32 : 1; // increment vram address by 1 if bit 2 of control register is 0, else increment by 32
         break;
      case PPUSTATUS: // read only
         break;
   }

   open_bus = data; // writes to any ppu ports loads a value into the I/O bus
}

uint8_t ppu_cpu_read(uint16_t position)
{
   switch(position)
   {
      case PPUCTRL: // write only
      case PPUMASK:
      case OAMADDR:
      case PPUSCROLL:
      case PPUADDR:
      case OAMDMA:
         break;
      case OAMDATA: // read/write
         open_bus = oam_data;
         break;
      case PPUDATA:
         open_bus = vram_buffer;
         vram_buffer = cartridge_ppu_read(vram_address);
         
         // when reading palette, data is returned directly from palette ram rather than the internal read buffer
         if ( vram_address >= PALETTE_START )
         {
            uint8_t palette_data = palette_ram[ vram_address & 0x1F ];
            return palette_data;
         }

         vram_address += (ppu_control & 4) ? 32 : 1; // increment vram address by 1 if bit 2 of control register is 0, else increment by 32
         break;
      case PPUSTATUS: // read only
         open_bus = (ppu_status & 0xE0) | open_bus;
         write_toggle = false;
         break;
   }

   return open_bus;
}

uint8_t* get_ppu_vram()
{
   return (uint8_t*) &vram;
}