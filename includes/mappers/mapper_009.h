#ifndef MAPPER_009_H
#define MAPPER_009_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <cartridge.h>

typedef struct Registers_009
{
	uint8_t prg_bank;

	uint8_t chr_bank_FD_0;
	uint8_t chr_bank_FE_0;
	uint8_t chr_bank_FD_1;
	uint8_t chr_bank_FE_1;

	uint8_t latch_0;
	uint8_t latch_1;

	uint8_t mirroring_mode; // 0: vertical, 1: horizontal
} Registers_009;

cartridge_access_mode_t mapper009_cpu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper009_cpu_write(nes_header_t* header, uint16_t position, uint8_t data, size_t* mapped_addr, void* internal_registers);

cartridge_access_mode_t mapper009_ppu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper009_ppu_write(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);


void mapper009_init(nes_header_t* header, void* internal_registers);

#endif