#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include "vec4.h"

typedef struct input_sprite_t
{
   uint8_t sprite_id;
   uint8_t y_coord;
   uint8_t tile_id;
   uint8_t attribute;
   uint8_t x_position;
} input_sprite_t;

typedef struct output_sprite_t
{
   uint8_t sprite_id;
   uint8_t x_position;
   uint8_t lo_bitplane;
   uint8_t hi_bitplane;
   uint8_t attribute;
} output_sprite_t;

/**
 * Resets ppu internal state.
*/
void ppu_reset(void);

/**
 * Init ppu state, useful when loading in new rom file
 */
void ppu_init(void);

/**
 * Loads .pal file into system colors for ppu.
 * @param path path to the .pal file
*/
bool ppu_load_palettes_from_file(const char* path);

void ppu_load_default_palettes();

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
void ppu_cycle(bool * nmi_flip_flop);

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

/// <summary>
/// Returns the state of oam dma flag to check if a dma is pending.
/// Will also clear the oam dma flag before returning its previous state.
/// </summary>
/// <returns>True if dma is scheduled, else false</returns>
bool ppu_scheduled_oam_dma(void);

/// <summary>
/// Execute oam dma process.
/// </summary>
void ppu_handle_oam_dma(void);

/**
 * Used by debug gui widget to view pattern tables. Updates pixel colors to draw current pixels inside
 * the pattern tables.
 * @param p0 color buffer of pixels that will be rendered for pattern table 0
 * @param p1 color buffer of pixels that will be rendered for pattern table 1
*/
void DEBUG_ppu_update_pattern_tables(vec4* p0, vec4* p1);

#endif
