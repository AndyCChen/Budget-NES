#ifndef PPU_H
#define PPU_H

#include <stdint.h>

/**
 * Lets cpu write data to a ppu register
 * @param position address of ppu register to write to
 * @param data content that is to be written to memory
*/
void ppu_cpu_write(uint16_t position, uint8_t data);

/**
 * Lets cpu read data from a ppu register
 * @param position address of ppu register to read from
 * @returns data that is read from ppu register
*/
uint8_t ppu_cpu_read(uint16_t position);

/**
 * Run the ppu for one cycle
*/
void ppu_cycle(void);

#endif