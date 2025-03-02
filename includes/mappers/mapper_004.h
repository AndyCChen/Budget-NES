#ifndef MAPPER_004_H
#define MAPPER_004_H

#include "cartridge.h"

// mapper 004
// https://www.nesdev.org/wiki/MMC3

typedef struct Registers_004
{
	uint8_t prg_rom_bank_mode;
	uint8_t chr_bank_mode;

	uint8_t register_select;   // 3 bit index to select which bank register to update
	uint8_t bank_registers[8]; // 8 bank registers for chr and prg banks

	uint8_t mirroring_mode;    // 0: vertical, 1: horizontal

	uint8_t prg_ram_enable;    // 0: disable ram, 1: enable ram
	uint8_t prg_write_disable; // 0: enable writes, 1: deny writes

	uint8_t irq_counter_reload;
	uint8_t irq_counter;
	bool    irq_enable;
	bool    irq_pending;
	uint8_t A12;
	size_t  M2; // number of cpu cycles where A12 remained low
	size_t  cpu_timestamp;
} Registers_004;

cartridge_access_mode_t mapper004_cpu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper004_cpu_write(nes_header_t* header, uint16_t position, uint8_t data, size_t* mapped_addr, void* internal_registers);

cartridge_access_mode_t mapper004_ppu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper004_ppu_write(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers);

bool mapper004_irq_signaled(void* internal_registers);

void mapper004_init(nes_header_t* header, void* internal_registers);

#endif