#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdbool.h>

#define iNES_HEADER_SIZE 16 // iNES headers are all 16 bytes long
#define CARTRIDGE_PRG_START 0x8000 // starting address of program rom

typedef struct mapper_t
{
   uint8_t (*cpu_read) (uint16_t);
   uint8_t (*ppu_read) (uint16_t);
   void (*cpu_write) (uint16_t, uint8_t);
   void (*ppu_write) (uint16_t, uint8_t);
} mapper_t;

/**
 * Loads correct mapper that contains pointers to cpu/ppu read/write
 * function depending on the iNES header.
*/
void load_mapper();

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
 * Frees the allocated memory for program and chr roms.
*/
void cartridge_free_memory();

#endif