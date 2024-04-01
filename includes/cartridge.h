#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>

// enum to signify which device on the cartridge is being accessed
typedef enum cartridge_access_mode_t
{
   ACCESS_PRG_ROM,
   ACCESS_PRG_RAM,
   ACCESS_CHR_MEM,     // CHR_MEM can represent either chr-ram or chr-rom
   ACCESS_VRAM,        // cartridge is able to map the location of ppu's vram
   NO_CARTRIDGE_DEVICE // adressed location has no active devices
} cartridge_access_mode_t;

typedef struct nes_header_t 
{
   uint8_t trainer;
   uint32_t prg_rom_size;         // in 16kb units
   uint32_t prg_ram_size;         // in 8kb units
   uint32_t chr_rom_size;         // in 8kb units
   uint16_t mapper_id;
   uint8_t nametable_arrangement; // 0: horizontal mirroring, 1: vertical mirroring
} nes_header_t;

/**
 * Loads program rom in cartridge into memory and
 * determines cartridge mapper that is to be used.
 * Also allocates memory for program rom/ram and chr rom/ram.
 */ 
bool cartridge_load(const char* const romPath);

/**
 * lets cpu read data from the cartridge
 * @param position location to read data from
 * @returns data that is read, will return data from previous read if addressed location has no devices
*/
uint8_t cartridge_cpu_read(uint16_t position);

/**
 * ppu reads are intercepted by cartridge mapper which decides how to map physical memory to addresses.
 * Exception is the palette ram which is non-configurable.
 * @param position location to read data from
 * @returns data that is read, will return data from previous read if addressed location has no devices.
*/
uint8_t cartridge_ppu_read(uint16_t position);

/**
 * Cartridge determines where ppu writes are mapped to.
 * @param position location to write data to
 * @param data data to write
*/
void cartridge_ppu_write(uint16_t position, uint8_t data);

/**
 * Frees the allocated memory for program and chr rom/ram.
*/
void cartridge_free_memory();

#endif