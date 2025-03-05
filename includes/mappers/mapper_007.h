#ifndef MAPPER_007_H
#define MAPPER_007_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mapper.h"

typedef struct Registers_007
{
	uint8_t prg_bank;
	uint8_t vram_bank; 
} Registers_007;

cartridge_access_mode_t mapper007_cpu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper007_cpu_write(nes_header_t* header, uint16_t position, uint8_t data, size_t* mapped_addr, void* internal_registers);

cartridge_access_mode_t mapper007_ppu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper007_ppu_write(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);

void mapper007_init(nes_header_t* header, void* internal_registers);


#endif
