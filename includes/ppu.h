#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>

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

void fetch_sprite_lo(void);
void fetch_sprite_hi(void);
void secondary_oam_clear(void);
void evaluate_sprite(void);

#endif