#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include <stdbool.h>

#include "cartridge.h"

typedef struct mapper_t
{
   cartridge_access_mode_t (*cpu_read)     (nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers);
   cartridge_access_mode_t (*ppu_read)     (nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers);
   // cpu write function takes in data parameter which is the data to write, this can be intercepted by cartridge mapper to do stuff like bank switching for example
   cartridge_access_mode_t (*cpu_write)    (nes_header_t *header, uint16_t position, uint8_t data, size_t *mapped_addr, void* internal_registers);
   cartridge_access_mode_t (*ppu_write)    (nes_header_t *header, uint16_t position, size_t *mapped_addr, void* internal_registers);
   void                    (*init)         (nes_header_t* header, void* internal_registers); // function to initialize a mapper's register if necessary
	bool                    (*irq_signaled) (void* internal_registers);
} mapper_t;

/**
 * Loads correct mapper that contains pointers to cpu/ppu read/write
 * function depending on the iNES header.
 * @param mapper_id id of mapper to load
 * @param mapper pointer to mapper struct that will contain loaded function pointer for reads/writes
 * @param mapper_registers pointer to struct that will contain a mapper's internal registers
*/
bool load_mapper(uint32_t mapper_id, mapper_t *mapper, void** mapper_registers);

#endif
