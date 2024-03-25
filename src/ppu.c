#include <stdint.h>

#include "../includes/ppu.h"

#define PPU_RAM_SIZE  1024 * 2

// cpu mapped addresses of PPU registers at 0x2000 - 0x2007 and 0x4041

#define PPUCTRL   0x2000 // write
#define PPUMASK   0x2001 // write
#define PPUSTATUS 0x2002 // read
#define OAMADDR   0x2003 // write
#define OAMDATA   0x2004 // read/write
#define PPUSCROLL 0x2005 // write x2
#define PPUADDR   0x2006 // write x2
#define PPUDATA   0x2007 // read/write
#define OAMDMA    0x4041 // write

// ppu registers

static uint8_t ppu_control;
static uint8_t ppu_mask;
static uint8_t ppu_status;
static uint8_t oam_address;
static uint8_t oam_data;
static uint8_t ppu_scroll;
static uint8_t ppu_address;
static uint8_t ppu_data;
static uint8_t oam_dma;

static uint8_t vram[PPU_RAM_SIZE];
static uint8_t palette[32];

static uint8_t open_bus = 0;

void ppu_cpu_write(uint16_t position, uint8_t data)
{
   switch(position)
   {
      // write only
      case PPUCTRL:
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
         break;
      case PPUADDR:
         ppu_address = data;
         break;
      case OAMDMA:
         oam_dma = data;
         break;

      // read/write
      case OAMDATA:
         oam_data = data;
         break;
      case PPUDATA:
         ppu_data = data;
         break;

      // read only
      case PPUSTATUS:
         break;
   }

   open_bus = data; // writes to any ppu ports loads a value into the I/O bus
}

uint8_t ppu_cpu_read(uint16_t position)
{
   switch(position)
   {
      // write only
      case PPUCTRL:
      case PPUMASK:
      case OAMADDR:
      case PPUSCROLL:
      case PPUADDR:
      case OAMDMA:
         break;

      // read/write
      case OAMDATA:
         open_bus = oam_data;
         break;
      case PPUDATA:
         open_bus = ppu_data;
         break;

      // read only
      case PPUSTATUS:
         open_bus = (ppu_status & 0xE0) | open_bus;
         break;
   }

   return open_bus;
}

void ppu_cycle(void)
{
   
}