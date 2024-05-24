#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include "vec4.h"

typedef struct output_sprite_t
{
   uint8_t x_position;
   uint8_t lo_bitplane;
   uint8_t hi_bitplane;
   uint8_t attribute;
} output_sprite_t;

/**
 * Loads .pal file into system colors for ppu.
 * @param path path to the .pal file
*/
bool ppu_load_palettes(const char* path);

/**
 * Check if nmi has occured
*/
bool get_nmi_status(void);

/**
 * Lets cpu write data throught the ppu ports
 * @param position address of ppu register to write to
 * @param data content that is to be written to memory
*/
void ppu_port_write(uint16_t position, uint8_t data);

/**
 * Lets cpu read data through the ppu ports
 * @param position address of ppu port to read from
 * @returns data in the internal read buffer, or returns data directly from from palette reading from that location
*/
uint8_t ppu_port_read(uint16_t position);

/**
 * Run the ppu for one cycle
*/
void ppu_cycle(void);

// render pipeline events

void rest_cycle(void);
void fetch_nametable(void);
void fetch_attribute(void);
void fetch_pattern_table_lo(void);
void fetch_pattern_table_hi(void);
void increment_v_horizontal(void);
void increment_v_vertical(void);
void increment_v_both(void);     // wrapper to call horizonal and vertical increment in one event
void transfer_t_horizontal(void);
void transfer_t_vertical(void);
void fetch_sprites(void);
void sprite_clear_secondary_oam(void);

/**
 * Used by debug gui widget to view pattern tables. Initializes pixel colors to draw based on values read
 * from the pattern tables. For chr-rom calling this once at the beginning will suffice. For chr-ram, since that changes
 * during runtime the pixel colors will need to also be update which is not currently handled yet.
 * @param p0 color buffer of pixels that will be rendered for pattern table 0
 * @param p1 color buffer of pixels that will be rendered for pattern table 1
*/
void DEBUG_ppu_init_pattern_tables(vec4* p0, vec4* p1);

#endif
