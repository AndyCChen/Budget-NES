#ifndef PPU_H
#define PPU_H

#include <stdint.h>

/**
 * Lets cpu write data throught the ppu ports
 * @param position address of ppu register to write to
 * @param data content that is to be written to memory
*/
void ppu_cpu_write(uint16_t position, uint8_t data);

/**
 * Lets cpu read data through the ppu ports
 * @param position address of ppu port to read from
 * @returns data in the internal read buffer, or returns data directly from from palette reading from that location
*/
uint8_t ppu_cpu_read(uint16_t position);

/**
 * Run the ppu for one cycle
*/
void ppu_cycle(void);

/**
 * @returns ppu vram array address
*/
uint8_t* get_ppu_vram();

#endif