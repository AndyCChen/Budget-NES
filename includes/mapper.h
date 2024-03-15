#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include <stdbool.h>

#include "../includes/cartridge.h"

typedef struct mapper_t
{
   cartridge_access_mode_t (*cpu_read) (nes_header_t *header, uint16_t position, uint16_t *mapped_addr);
   cartridge_access_mode_t (*ppu_read) (nes_header_t *header, uint16_t position, uint16_t * mapped_addr);
   cartridge_access_mode_t (*cpu_write) (nes_header_t *header, uint16_t position, uint16_t *mapped_addr);
   cartridge_access_mode_t (*ppu_write) (nes_header_t *header, uint16_t position, uint16_t *mapped_addr);
} mapper_t;

/**
 * Loads correct mapper that contains pointers to cpu/ppu read/write
 * function depending on the iNES header.
 * @param mapper_id id of mapper to load
 * @param mapper pointer to mapper struct that will contain loaded function pointer for reads/writes
*/
bool load_mapper(uint32_t mapper_id, mapper_t *mapper);

#endif