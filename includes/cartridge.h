#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>
#include <stdint.h>

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
 * Allocates memory for cartridge data and loads data into allocated memory.
 */
bool cartridge_load(const char* const romPath);

/**
 * lets cpu read data from the cartridge
 * @param position location to read data from
 * @returns data that is read, will return data from previous read if addressed location has no devices
*/
uint8_t cartridge_cpu_read(uint16_t position);

/**
 * lets cpu write data to the cartridge
 * @param position location to write data to
 * @param data data to write
*/
void cartridge_cpu_write(uint16_t position, uint8_t data);

/**
 * Called by the ppu when reading from cartridge or memory that can be configured by the cartridge mapper.
 * Exception is the palette ram which is non-configurable.
 * @param position location to read data from
 * @param ppu_vram pointer to 2kb ppu vram so cartridge can directly read from vram after mapping the incoming address
 * @returns data that is read, will return data from previous read if addressed location has no devices.
*/
uint8_t cartridge_ppu_read(uint16_t position);

/**
 * Called by the ppu when writing to cartridge or memory that can be configured by the cartridge mapper.
 * @param position location to write data to
 * @param data data to write, if writing to non writable location the write will not occur
 * @param ppu_vram pointer to 2kb ppu vram so cartridge can directly write to vram after mapping the incoming address
*/
void cartridge_ppu_write(uint16_t position, uint8_t data);

/**
 * Frees the allocated memory for program and chr rom/ram.
*/
void cartridge_free_memory(void);

/**
 * Returns status of if pattern table viewer in debug gui should be updated.
 */
bool DEBUG_is_pattern_updated(void);

/**
 * Set status of if pattern table viewer display should be updated
 */
void DEBUG_trigger_pattern_table_update(void);

#endif
