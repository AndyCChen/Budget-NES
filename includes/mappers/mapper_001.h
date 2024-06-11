#ifndef MAPPER_001_H
#define MAPPER_001_H

#include <stdint.h>
#include <stdbool.h>

#include "../cartridge.h"

/**
 * Internal registers of mapper 001.
 * https://www.nesdev.org/wiki/MMC1
 */
typedef struct Registers_001
{
   uint8_t shift_register;
   uint8_t control;
   uint8_t CHR_bank_0;
   uint8_t CHR_bank_1;
   uint8_t PRG_bank;
} Registers_001;

cartridge_access_mode_t mapper001_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper001_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, uint16_t *mapped_addr, void* internal_registers);

cartridge_access_mode_t mapper001_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers);
cartridge_access_mode_t mapper001_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers);

void mapper001_init(nes_header_t* header, void* internal_registers);

#endif
