#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>

typedef enum cartridge_mode_t
{
   PROGRAM_ROM,
   CHR_ROM,
   CHR_RAM,
   PROGRAM_RAM
} cartridge_mode_t;

typedef struct nes_header_t 
{
   uint8_t trainer;
   uint32_t program_rom_size; // in 16kb units
   uint32_t chr_rom_size;     // in 8kb units
   uint32_t mapper_id;
} nes_header_t;

/**
 * Loads program rom in cartridge into memory and
 * determines cartridge mapper that is to be used.
 * Also allocated memory for program and chr rom.
 */ 
bool cartridge_load(const char* const romPath);

/**
 * lets cpu read data from the cartridge
 * @param position location to read data from
 * @returns data that is read
*/
uint8_t cartridge_cpu_read(uint16_t position);

/**
 * Lets ppu read data from cartridge.
 * @param position location to read data from
 * @returns data that is read
*/
uint8_t cartridge_ppu_read(uint16_t position);

/**
 * Frees the allocated memory for program and chr roms.
*/
void cartridge_free_memory();

#endif